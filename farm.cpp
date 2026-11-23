#include "farm.h"
#include "sdmon.h"
#include <FS.h>
#include <SD_MMC.h>

// ~24 KB: en PSRAM (se reserva en farmLoad), la RAM interna la necesitan TLS y Wi-Fi
static FarmEntry *entries = nullptr;
static int count = 0;

static void farmPath(char *buf, size_t n, int16_t dex) {
  snprintf(buf, n, "/farm/f%04d.bin", dex);
}

static void setBit(Pet &p, int16_t dex, bool on) {
  uint8_t &b = p.farmReg[(dex - 1) >> 3];
  uint8_t m = (uint8_t)(1 << ((dex - 1) & 7));
  b = on ? (b | m) : (b & ~m);
}

// v3 (actual), v2 (sin farmEpoch) o v1 (tampoco bornEpoch): las viejas son mas
// cortas y lo que falta queda en 0
static bool readRec(const char *path, PetRec &r) {
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;
  memset(&r, 0, sizeof(r));
  size_t n = f.size();
  uint8_t ver = n == sizeof(PetRec) ? PET_REC_VER : n == PET_REC_V3_SIZE ? 3 : n == PET_REC_V2_SIZE ? 2
              : n == PET_REC_V1_SIZE ? 1 : 0;
  bool ok = ver && f.read((uint8_t *)&r, n) == n && r.ver == ver && r.speciesId >= 1 && r.speciesId <= PET_DEX_MAX;
  f.close();
  return ok;
}

// primero a un fichero aparte (fuera de /farm: farmLoad no lo ve) y luego se cambia por el bueno: si
// la escritura falla (tarjeta llena, un corte), el que ya habia sigue entero
#define FARM_TMP "/farm.tmp"

// lo que dejo un writeRec cortado (un corte, o un rename que fallo, entre borrar la ficha y renombrar la
// nueva). Si su especie se quedo sin fichero, la nueva, que ya estaba entera, pasa a serlo; si no,
// sobra (la vieja sigue ahi: el cambio no llego a hacerse) y se borra, que mas tarde no reviva a un
// pokemon que ya se saco de la granja
static void farmRecoverTmp() {
  if (!SD_MMC.exists(FARM_TMP)) return;
  PetRec t;
  if (readRec(FARM_TMP, t)) {
    char path[32];
    farmPath(path, sizeof(path), t.speciesId);
    if (!SD_MMC.exists(path) && SD_MMC.rename(FARM_TMP, path)) {
      Serial.println("granja: recuperada una ficha a medio guardar");
      return;
    }
  }
  SD_MMC.remove(FARM_TMP);
}

static bool writeRec(const char *path, const PetRec &r) {
  farmRecoverTmp();  // antes de reutilizar /farm.tmp: podria ser la unica copia de alguien
  File f = SD_MMC.open(FARM_TMP, FILE_WRITE);
  if (!f) return false;
  bool ok = f.write((const uint8_t *)&r, sizeof(r)) == sizeof(r);
  f.close();
  if (!ok) {
    SD_MMC.remove(FARM_TMP);
    return false;
  }
  SD_MMC.remove(path);  // FAT no renombra encima de otro fichero
  return SD_MMC.rename(FARM_TMP, path);
}

static FarmEntry entryOf(const PetRec &r) {
  FarmEntry e;
  e.speciesId = r.speciesId;
  e.shiny = r.shiny;
  e.ageMinutes = r.ageMinutes;
  e.farmEpoch = r.farmEpoch;
  e.evoCount = r.ver >= 4 ? r.evoCount : 0;  // como importRec
  memcpy(e.nick, r.nick, sizeof(e.nick));
  e.nick[sizeof(e.nick) - 1] = 0;
  return e;
}

// inserta manteniendo el orden por especie (reemplaza si ya estaba)
static void indexPut(const FarmEntry &e) {
  if (!entries) return;
  int i = 0;
  while (i < count && entries[i].speciesId < e.speciesId) i++;
  if (i < count && entries[i].speciesId == e.speciesId) {
    entries[i] = e;
    return;
  }
  if (count >= PET_DEX_MAX) return;
  memmove(&entries[i + 1], &entries[i], (count - i) * sizeof(FarmEntry));
  entries[i] = e;
  count++;
}

static void indexDel(int16_t dex) {
  for (int i = 0; i < count; i++) {
    if (entries[i].speciesId != dex) continue;
    memmove(&entries[i], &entries[i + 1], (count - i - 1) * sizeof(FarmEntry));
    count--;
    return;
  }
}

bool farmReady() { return sdReady; }

bool farmLoad(Pet &p) {
  count = 0;
  memset(p.farmReg, 0, sizeof(p.farmReg));
  if (!sdReady) return false;
  if (!entries) entries = (FarmEntry *)ps_malloc(PET_DEX_MAX * sizeof(FarmEntry));
  if (!entries) return false;
  SD_MMC.mkdir("/farm");
  farmRecoverTmp();  // antes de leer /farm: una ficha que se quedo a medio cambiar
  File dir = SD_MMC.open("/farm");
  if (!dir) return false;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    char path[32];
    snprintf(path, sizeof(path), "/farm/%s", f.name());
    f.close();
    PetRec r;
    if (!readRec(path, r)) continue;
    if (r.ver < PET_REC_VER) {  // ficha vieja: se pasa a la version actual
      if (r.ver < 3) r.farmEpoch = p.savedEpoch();  // de antes de crecer en la granja: crece desde ahora
      r.ver = PET_REC_VER;
      writeRec(path, r);
    }
    indexPut(entryOf(r));
    setBit(p, r.speciesId, true);
  }
  dir.close();
  Serial.printf("granja: %d\n", count);
  return true;
}

int farmCount() { return count; }

const FarmEntry &farmAt(int i) { return entries[i]; }

bool farmPut(Pet &p, const PetRec &r) {
  if (!sdReady || r.speciesId < 1 || r.speciesId > PET_DEX_MAX) return false;
  char path[32];
  farmPath(path, sizeof(path), r.speciesId);
  if (!writeRec(path, r)) return false;  // el que hubiera de esta especie sigue como estaba
  indexPut(entryOf(r));
  setBit(p, r.speciesId, true);
  return true;
}

bool farmPeek(int16_t dex, PetRec &out) {
  if (!sdReady || dex < 1 || dex > PET_DEX_MAX) return false;
  char path[32];
  farmPath(path, sizeof(path), dex);
  return readRec(path, out);
}

bool farmTake(Pet &p, int16_t dex, PetRec &out) {
  if (!sdReady || dex < 1 || dex > PET_DEX_MAX) return false;
  char path[32];
  farmPath(path, sizeof(path), dex);
  if (!readRec(path, out)) return false;
  SD_MMC.remove(path);
  indexDel(dex);
  setBit(p, dex, false);
  return true;
}
