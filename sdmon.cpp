#include "sdmon.h"
#include "pin_config.h"
#include "voice.h"
#include <FS.h>
#include <SD_MMC.h>

bool sdReady = false;
bool sdDirty = false;
SdThumbs thumbs;
SdBg bg;

bool PmdMon::load(uint16_t dexNum, bool shiny, bool mega) {
  unload();
  if (!sdReady) return false;

  char path[28];
  const char *pre = mega ? "m" : "p";
  snprintf(path, sizeof(path), "/mons/%s%s%03u.bin", pre, shiny ? "s" : "", dexNum);
  File f = SD_MMC.open(path, FILE_READ);
  if (!f && shiny) {  // sin shiny PMD: usa el normal
    snprintf(path, sizeof(path), "/mons/%s%03u.bin", pre, dexNum);
    f = SD_MMC.open(path, FILE_READ);
  }
  if (!f) return false;

  uint32_t size = f.size();
  if (size < 7 || size > 3UL * 1024 * 1024) { f.close(); return false; }
  blob = (uint8_t *)ps_malloc(size);
  if (!blob || f.read(blob, size) != size || memcmp(blob, "TPK2", 4) != 0) {
    if (blob) { free(blob); blob = nullptr; }
    f.close();
    return false;
  }
  f.close();

  uint8_t nActs = blob[4];
  memcpy(&palCount, blob + 5, 2);
  if (palCount > 256 || (uint32_t)7 + palCount * 2 > size) { unload(); return false; }
  memcpy(pal, blob + 7, palCount * 2);

  const uint8_t *p = blob + 7 + palCount * 2;
  const uint8_t *end = blob + size;
  for (uint8_t i = 0; i < nActs && p + 4 <= end; i++) {
    uint8_t id = p[0], w = p[1], h = p[2], nf = p[3];
    p += 4;
    if (id >= PMD_NACTS || nf > 24) { unload(); return false; }
    // valida que ms[] y los datos del frame caben en el blob (archivo truncado)
    uint32_t bytes = (uint32_t)nf * 2 + (uint32_t)w * h * nf;
    if (w == 0 || h == 0 || nf == 0 || p + bytes > end) { unload(); return false; }
    PmdAct &a = acts[id];
    a.w = w;
    a.h = h;
    a.frames = nf;
    for (uint8_t k = 0; k < nf; k++) {
      a.ms[k] = p[0] | (p[1] << 8);
      if (a.ms[k] == 0) a.ms[k] = 100;  // nunca 0: pmdFrameAt() giraria sin avanzar
      p += 2;
    }
    a.data = p;
    p += (uint32_t)w * h * nf;
    // fila mas baja con contenido en cualquier frame: anclar por los pies
    uint8_t base = 1;
    for (uint8_t f = 0; f < nf; f++) {
      const uint8_t *fr = a.data + (uint32_t)f * w * h;
      for (int r = h - 1; r >= 0; r--) {
        bool any = false;
        for (int c = 0; c < w && !any; c++)
          if (fr[r * w + c] != 0xFF) any = true;
        if (any) { if (r + 1 > base) base = r + 1; break; }
      }
    }
    a.base = base;
  }
  // alto real del bicho quieto (primer frame de Idle), sin el margen del lienzo
  idleH = 0;
  const PmdAct &q = acts[PMD_IDLE];
  int r0 = -1, r1 = -1;
  for (int r = 0; q.frames && r < q.h; r++)
    for (int c = 0; c < q.w; c++)
      if (q.data[r * q.w + c] != 0xFF) { if (r0 < 0) r0 = r; r1 = r; break; }
  if (r0 >= 0) idleH = r1 - r0 + 1;
  loaded = true;
  Serial.printf("cargado %s (%u KB)\n", path, size / 1024);
  return true;
}

void PmdMon::unload() {
  if (blob) {
    free(blob);
    blob = nullptr;
  }
  for (auto &a : acts) {
    a.w = a.h = a.frames = a.base = 0;
    a.data = nullptr;
  }
  loaded = false;
}

void SdThumbs::unload() {
  if (data) { free(data); data = nullptr; }
  loaded = false;
  count = 0;
  size = 0;
}

bool SdThumbs::load() {
  unload();  // recargar sin fugar el blob anterior
  if (!sdReady) return false;
  File f = SD_MMC.open("/mons/thumbs.bin", FILE_READ);
  if (!f) {
    Serial.println("sin thumbs.bin (galeria sin miniaturas)");
    return false;
  }
  uint32_t sz = f.size();
  // acota el tamano: evita un ps_malloc absurdo con un archivo corrupto
  if (sz < 10 || sz > 2UL * 1024 * 1024) {
    Serial.println("thumbs.bin invalido (tamano)");
    f.close();
    return false;
  }
  data = (uint8_t *)ps_malloc(sz);
  if (!data || f.read(data, sz) != sz || memcmp(data, "TPTH", 4) != 0) {
    Serial.println("thumbs.bin invalido");
    if (data) { free(data); data = nullptr; }
    f.close();
    return false;
  }
  f.close();
  memcpy(&count, data + 4, 2);
  // la tabla de offsets (uno por especie) debe caber entera en lo leido
  if ((uint32_t)6 + 4UL * count > sz) {
    Serial.println("thumbs.bin invalido (tabla de offsets)");
    unload();
    return false;
  }
  size = sz;
  loaded = true;
  Serial.printf("miniaturas cargadas: %u (%u KB)\n", count, sz / 1024);
  return true;
}

const uint8_t *SdThumbs::get(int16_t dex) const {
  if (!loaded || dex < 1 || dex > count) return nullptr;
  uint32_t off;
  memcpy(&off, data + 6 + 4 * (dex - 1), 4);
  // 0 (o cualquier offset dentro de la cabecera) = especie sin miniatura
  if (off < 6 + 4UL * count) return nullptr;
  // el offset viene del fichero sin comprobar: sin esto, un thumbs.bin truncado
  // o corrupto hacia leer fuera de la reserva de PSRAM
  if (off > size - 3) return nullptr;  // la cabecera w,h,palCount debe caber (size >= 10 siempre)
  uint32_t need = 3 + (uint32_t)data[off + 2] * 2 + (uint32_t)data[off] * data[off + 1];
  if (need > size || off > size - need) return nullptr;  // y el blob entero tambien
  return data + off;
}

bool sdBegin() {
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
  // nunca se formatea: un fallo pasajero al montar (un contacto) borraba la copia, la granja y el album.
  // Si no monta, se sigue sin tarjeta y se formatea a FAT32 en el ordenador
  sdReady = SD_MMC.begin("/sdcard", true /* modo 1-bit */, false /* no formatea si no monta */);
  if (sdReady) {
    Serial.printf("SD montada: %llu MB\n", SD_MMC.cardSize() / (1024ULL * 1024ULL));
    SD_MMC.mkdir("/mons");
  } else {
    Serial.println("SD no detectada (el juego usa los sprites de flash)");
  }
  return sdReady;
}

bool SdMon::load(uint16_t dexNum, bool shiny) {
  unload();
  if (!sdReady) return false;

  char path[24];
  snprintf(path, sizeof(path), "/mons/%s%03u.bin", shiny ? "s" : "", dexNum);
  File f = SD_MMC.open(path, FILE_READ);
  if (!f && shiny) {  // sin variante shiny: usa la normal
    snprintf(path, sizeof(path), "/mons/%03u.bin", dexNum);
    f = SD_MMC.open(path, FILE_READ);
  }
  if (!f) {
    Serial.printf("no existe %s\n", path);
    return false;
  }

  char magic[4];
  uint16_t header[4];
  if (f.read((uint8_t *)magic, 4) != 4 || memcmp(magic, "TPK1", 4) != 0 ||
      f.read((uint8_t *)header, 8) != 8) {
    f.close();
    return false;
  }
  w = header[0];
  h = header[1];
  frames = header[2];
  frameMs = header[3];
  // acota dimensiones: evita size desbordado o absurdo con archivo corrupto
  if (f.read((uint8_t *)&palCount, 2) != 2 || palCount > 256 ||
      w == 0 || w > 256 || h == 0 || h > 256 || frames == 0 || frames > 64) {
    f.close();
    return false;
  }
  if (f.read((uint8_t *)pal, palCount * 2) != palCount * 2) {
    f.close();
    return false;
  }

  uint32_t size = (uint32_t)w * h * frames;
  data = (uint8_t *)ps_malloc(size);
  if (!data) {
    Serial.println("sin PSRAM para el sprite");
    f.close();
    return false;
  }
  uint32_t got = f.read(data, size);
  f.close();
  if (got != size) {
    Serial.printf("%s truncado (%u de %u)\n", path, got, size);
    unload();
    return false;
  }

  // zoom entero para que el bicho mida ~200 px de alto en pantalla
  scale = 200 / h;
  if (scale < 2) scale = 2;
  if (scale > 5) scale = 5;

  Serial.printf("cargado %s: %ux%u x%u frames @%ums, escala %u\n",
                path, w, h, frames, frameMs, scale);
  loaded = true;
  return true;
}

void SdMon::unload() {
  if (data) {
    free(data);
    data = nullptr;
  }
  loaded = false;
}

// ---------------------------------------------------------------------------
// Protocolo de carga por USB (para llenar la SD sin sacarla de la placa):
//   PUT <ruta> <bytes>\n  + datos crudos   -> "OK" ... "DONE"   (en /mons/ o /voice/)
//   LS\n                                   -> listado de /mons   (LS /voice/<amigo>: sus grabaciones)
// Usar con tools/send_sd.py
// ---------------------------------------------------------------------------

bool sdSerialCommand(const String &line) {
  if (line.startsWith("PUT ")) {
    int sp = line.lastIndexOf(' ');
    String path = line.substring(4, sp);
    uint32_t size = line.substring(sp + 1).toInt();
    if (!sdReady || size == 0 || size > 4 * 1024 * 1024) {
      Serial.println("ERR");
      return true;
    }
    if (!path.startsWith("/")) path = "/" + path;
    // acota la escritura a /mons/: la ruta llega tal cual de la linea serie, sin
    // sanear, asi que un PUT manipulado podria escribir en cualquier sitio de la
    // tarjeta. Los dos clientes reales (tools/send_sd.py y web/index.html) ya
    // mandan nombres con el prefijo mons/.
    // y a /voice/ (las grabaciones de los amigos, voice.h), cuyas carpetas se crean al subir la primera
    bool voz = path.startsWith("/voice/");
    if (!(path.startsWith("/mons/") || voz) || path.indexOf("..") >= 0) {
      Serial.println("ERR");
      return true;
    }
    for (int i = path.indexOf('/', 1); voz && i > 0; i = path.indexOf('/', i + 1)) SD_MMC.mkdir(path.substring(0, i));
    // FILE_WRITE ANADE al final si el fichero ya existe, asi que reintentar uno
    // que quedo a medias lo alargaba en vez de reemplazarlo: quedaba un sprite
    // corrupto y mas grande que el original. Importa mas desde que el instalador
    // reanuda transferencias cortadas, porque reintenta justo los que fallaron.
    if (SD_MMC.exists(path)) SD_MMC.remove(path);
    File f = SD_MMC.open(path, FILE_WRITE);
    if (!f) {
      Serial.println("ERR");
      return true;
    }
    Serial.println("OK");
    static uint8_t buf[2048];
    uint32_t remaining = size;
    Serial.setTimeout(5000);
    while (remaining > 0) {
      size_t want = remaining > sizeof(buf) ? sizeof(buf) : remaining;
      size_t n = Serial.readBytes(buf, want);
      if (n == 0) break;  // timeout
      // sin mirar el retorno, una tarjeta llena o con fallo de escritura daba
      // DONE igualmente y dejaba el fichero truncado en la SD: justo la entrada
      // corrupta contra la que hay que protegerse luego al cargar el sprite
      if (f.write(buf, n) != n) break;
      remaining -= n;
      Serial.println("#");  // ack: listo para el siguiente bloque
    }
    f.close();
    Serial.setTimeout(1000);
    // a medias no sirve: un sprite o una voz cortados se cargarian igual (una voz sonaria a trozos)
    if (remaining != 0) SD_MMC.remove(path);
    sdDirty = (remaining == 0);
    Serial.println(remaining == 0 ? "DONE" : "ERR");
    return true;
  } else if (line == "SDINFO") {  // diagnostico remoto de "no me reconoce la SD"
    Serial.printf("total=%llu used=%llu\n", SD_MMC.totalBytes(), SD_MMC.usedBytes());
    Serial.println("DONE");
    return true;
  } else if (line == "LS" || (line.startsWith("LS /voice") && line.indexOf("..") < 0)) {
    String d = line == "LS" ? String("/mons") : line.substring(3);
    File dir = SD_MMC.open(d);
    if (dir) {
      File e;
      while ((e = dir.openNextFile())) {
        Serial.printf("%s %u\n", e.name(), (uint32_t)e.size());
        e.close();
      }
      dir.close();
    }
    Serial.println("DONE");
    return true;
  }
  return false;
}

int16_t *sdLoadVoice(uint64_t friendPid, int clip, uint32_t &samples) {
  samples = 0;
  if (!sdReady || clip < 0) return nullptr;
  char path[32];
  voicePath(path, sizeof(path), friendPid, clip);
  if (!SD_MMC.exists(path)) return nullptr;  // lo normal: ese amigo no grabo nada para esto
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return nullptr;
  uint8_t h[256];
  size_t hn = f.read(h, sizeof(h));
  uint32_t off, len;
  int16_t *pcm = nullptr;
  if (voiceWav(h, hn, (uint32_t)f.size(), off, len) && f.seek(off)) {
    pcm = (int16_t *)ps_malloc(len);
    if (pcm && f.read((uint8_t *)pcm, len) != len) {
      free(pcm);
      pcm = nullptr;
    }
  } else {
    Serial.printf("voz: %s no es WAV PCM 16 kHz mono 16 bits\n", path);
  }
  f.close();
  if (pcm) samples = len / 2;
  return pcm;
}

bool SdBg::load(int r, uint8_t luz) {
  if (px && region == r && tint == luz) return true;
  region = -1;
  if (!sdReady || r < 0) return false;
  char path[24];
  snprintf(path, sizeof(path), "/mons/bg%d.bin", r + 1);
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;
  uint8_t head[8];
  uint16_t fw = 0, fh = 0;
  if (f.read(head, 8) == 8 && memcmp(head, "TPBG", 4) == 0) {
    memcpy(&fw, head + 4, 2);
    memcpy(&fh, head + 6, 2);
  }
  uint32_t n = (uint32_t)fw * fh;
  // la medida tiene que ser la del fichero y caber en la escena (466 de ancho)
  if (fw == 0 || fh == 0 || fw > 466 || fh > 466 || f.size() != 8 + n * 2) {
    f.close();
    return false;
  }
  // la reserva se compara con lo que cabe, no con w*h: si una lectura fallaba, w y h se quedaban
  // con la medida de antes y la reserva ya era la nueva, y el siguiente fondo de esa medida vieja
  // se leia en un bufer mas pequeño (fuera de la reserva)
  if (!px || cap != n) {
    if (px) free(px);
    px = (uint16_t *)ps_malloc(n * 2);
    cap = px ? n : 0;
    if (!px) { f.close(); w = h = 0; return false; }
  }
  w = h = 0;  // a medio leer no vale (bgIsSky mira w y h)
  bool ok = f.read((uint8_t *)px, n * 2) == n * 2;
  f.close();
  if (!ok) return false;
  w = fw;
  h = fh;
  if (luz == BG_NIGHT) {  // de noche: a ~40-60 % y con un toque azul
    for (uint32_t i = 0; i < n; i++) {
      uint16_t v = px[i];
      uint16_t rr = ((v >> 11) & 31) * 2 / 5, gg = ((v >> 5) & 63) * 9 / 20, bb = (v & 31) * 3 / 5;
      px[i] = (rr << 11) | (gg << 5) | bb;
    }
  } else if (luz == BG_TWILIGHT) {  // amanecer/atardecer: calido, un poco mas oscuro
    for (uint32_t i = 0; i < n; i++) {
      uint16_t v = px[i];
      uint16_t rr = ((v >> 11) & 31) * 19 / 20, gg = ((v >> 5) & 63) * 7 / 10, bb = (v & 31) * 1 / 2;
      px[i] = (rr << 11) | (gg << 5) | bb;
    }
  }
  region = r;
  tint = luz;
  return true;
}
