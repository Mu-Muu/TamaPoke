#include "backup.h"
#include "sdmon.h"
#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <nvs.h>
#include <nvs_flash.h>

// formato: "TPBK", u8 version, u32 hora, u16 n entradas; y por entrada:
// u8 espacio, u8 tipo (nvs_type_t), u8 largo de la clave, clave, u16 largo, datos
#define BK_PATH "/backup.bin"
#define BK_TMP "/backup.tmp"
#define BK_VER 1
static const char *const NS[] = { "tamapoke", "chess", "fest" };
#define NS_COUNT 3

static bool put(File &f, const void *d, size_t n) { return f.write((const uint8_t *)d, n) == n; }
static bool get(File &f, void *d, size_t n) { return f.read((uint8_t *)d, n) == (int)n; }

// lee el valor de una entrada a buf (hasta cap): largo, 0 si falla
static size_t readValue(nvs_handle_t h, const char *key, nvs_type_t t, uint8_t *buf, size_t cap) {
  size_t n = cap;
  switch (t) {
    case NVS_TYPE_U8: return nvs_get_u8(h, key, (uint8_t *)buf) == ESP_OK ? 1 : 0;
    case NVS_TYPE_I8: return nvs_get_i8(h, key, (int8_t *)buf) == ESP_OK ? 1 : 0;
    case NVS_TYPE_U16: return nvs_get_u16(h, key, (uint16_t *)buf) == ESP_OK ? 2 : 0;
    case NVS_TYPE_I16: return nvs_get_i16(h, key, (int16_t *)buf) == ESP_OK ? 2 : 0;
    case NVS_TYPE_U32: return nvs_get_u32(h, key, (uint32_t *)buf) == ESP_OK ? 4 : 0;
    case NVS_TYPE_I32: return nvs_get_i32(h, key, (int32_t *)buf) == ESP_OK ? 4 : 0;
    case NVS_TYPE_U64: return nvs_get_u64(h, key, (uint64_t *)buf) == ESP_OK ? 8 : 0;
    case NVS_TYPE_I64: return nvs_get_i64(h, key, (int64_t *)buf) == ESP_OK ? 8 : 0;
    case NVS_TYPE_STR: return nvs_get_str(h, key, (char *)buf, &n) == ESP_OK ? n : 0;  // con el 0 final
    case NVS_TYPE_BLOB: return nvs_get_blob(h, key, buf, &n) == ESP_OK ? n : 0;
    default: return 0;
  }
}

static esp_err_t writeValue(nvs_handle_t h, const char *key, nvs_type_t t, const uint8_t *b, size_t n) {
  switch (t) {
    case NVS_TYPE_U8: return nvs_set_u8(h, key, *(const uint8_t *)b);
    case NVS_TYPE_I8: return nvs_set_i8(h, key, *(const int8_t *)b);
    case NVS_TYPE_U16: return nvs_set_u16(h, key, *(const uint16_t *)b);
    case NVS_TYPE_I16: return nvs_set_i16(h, key, *(const int16_t *)b);
    case NVS_TYPE_U32: return nvs_set_u32(h, key, *(const uint32_t *)b);
    case NVS_TYPE_I32: return nvs_set_i32(h, key, *(const int32_t *)b);
    case NVS_TYPE_U64: return nvs_set_u64(h, key, *(const uint64_t *)b);
    case NVS_TYPE_I64: return nvs_set_i64(h, key, *(const int64_t *)b);
    case NVS_TYPE_STR: return nvs_set_str(h, key, (const char *)b);
    case NVS_TYPE_BLOB: return nvs_set_blob(h, key, b, n);
    default: return ESP_ERR_INVALID_ARG;
  }
}

bool backupSave(uint32_t wallEpoch) {
  if (!sdReady) return false;
  static uint8_t buf[1024];  // el blob mas grande es la pokedex (129 bytes) y la mochila
  SD_MMC.remove(BK_TMP);
  File f = SD_MMC.open(BK_TMP, FILE_WRITE);
  if (!f) return false;
  uint8_t ver = BK_VER;
  uint16_t count = 0;
  bool ok = put(f, "TPBK", 4) && put(f, &ver, 1) && put(f, &wallEpoch, 4) && put(f, &count, 2);
  for (uint8_t ns = 0; ns < NS_COUNT && ok; ns++) {
    nvs_handle_t h;
    if (nvs_open(NS[ns], NVS_READONLY, &h) != ESP_OK) continue;  // espacio aun sin crear
    nvs_iterator_t it = nullptr;
    esp_err_t e = nvs_entry_find(NVS_DEFAULT_PART_NAME, NS[ns], NVS_TYPE_ANY, &it);
    while (e == ESP_OK && ok) {
      nvs_entry_info_t info;
      nvs_entry_info(it, &info);
      size_t n = readValue(h, info.key, info.type, buf, sizeof(buf));
      if (n) {
        uint8_t t = (uint8_t)info.type, kl = (uint8_t)strlen(info.key);
        uint16_t len = (uint16_t)n;
        ok = put(f, &ns, 1) && put(f, &t, 1) && put(f, &kl, 1) && put(f, info.key, kl) && put(f, &len, 2) &&
             put(f, buf, n);
        count++;
      }
      e = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    nvs_close(h);
  }
  if (ok) ok = f.seek(9) && put(f, &count, 2);  // el numero de entradas, ya sabido
  f.close();
  if (!ok || count == 0) {
    SD_MMC.remove(BK_TMP);
    return false;
  }
  SD_MMC.remove(BK_PATH);  // solo una copia: la nueva sustituye a la anterior
  ok = SD_MMC.rename(BK_TMP, BK_PATH);
  Serial.printf("copia: %u entradas %s\n", count, ok ? "guardada" : "FALLO");
  return ok;
}

static bool openBackup(File &f, uint32_t &epoch, uint16_t &count) {
  if (!sdReady) return false;
  f = SD_MMC.open(BK_PATH, FILE_READ);
  if (!f) return false;
  char mag[4];
  uint8_t ver;
  if (!get(f, mag, 4) || memcmp(mag, "TPBK", 4) || !get(f, &ver, 1) || ver != BK_VER || !get(f, &epoch, 4) ||
      !get(f, &count, 2)) {
    f.close();
    return false;
  }
  return true;
}

bool backupInfo(uint32_t &wallEpoch) {
  File f;
  uint16_t count;
  if (!openBackup(f, wallEpoch, count)) return false;
  f.close();
  return count > 0;
}

bool backupRestore() {
  File f;
  uint32_t epoch;
  uint16_t count;
  if (!openBackup(f, epoch, count) || count == 0) return false;
  static uint8_t buf[1024];
  // se lee entero antes de tocar nada: si el fichero esta roto, la partida actual sigue intacta
  struct Ent { uint8_t ns, type; char key[16]; uint16_t len; uint32_t off; };
  // en PSRAM y solo mientras se restaura (14 KB que en la RAM interna hacen falta a TLS)
  const uint16_t ENT_CAP = 256;
  const uint32_t DATA_CAP = 8192;
  Ent *ents = (Ent *)ps_malloc(ENT_CAP * sizeof(Ent));
  uint8_t *data = (uint8_t *)ps_malloc(DATA_CAP);
  uint32_t used = 0;
  bool ok = ents && data && count <= ENT_CAP;
  for (uint16_t i = 0; i < count && ok; i++) {
    Ent &e = ents[i];
    uint8_t kl;
    ok = get(f, &e.ns, 1) && get(f, &e.type, 1) && get(f, &kl, 1) && kl < sizeof(e.key) && e.ns < NS_COUNT &&
         get(f, e.key, kl) && get(f, &e.len, 2) && e.len <= sizeof(buf) && used + e.len <= DATA_CAP &&
         get(f, data + used, e.len);
    e.key[kl] = 0;
    e.off = used;
    used += e.len;
  }
  f.close();
  for (uint8_t ns = 0; ok && ns < NS_COUNT; ns++) {
    nvs_handle_t h;
    if (nvs_open(NS[ns], NVS_READWRITE, &h) != ESP_OK) {
      ok = false;
      break;
    }
    nvs_erase_all(h);
    for (uint16_t i = 0; i < count; i++)
      if (ents[i].ns == ns) writeValue(h, ents[i].key, (nvs_type_t)ents[i].type, data + ents[i].off, ents[i].len);
    nvs_commit(h);
    nvs_close(h);
  }
  free(ents);
  free(data);
  if (ok) Serial.printf("copia restaurada: %u entradas\n", count);
  return ok;
}
