#include "memoir.h"
#include "sdmon.h"
#include <FS.h>
#include <SD_MMC.h>

#define MEMOIR_PATH "/memoir.bin"

bool memoirAdd(const MemRec &r) {
  if (!sdReady) return false;
  File f = SD_MMC.open(MEMOIR_PATH, FILE_APPEND);
  if (!f) return false;
  bool ok = f.write((const uint8_t *)&r, sizeof(r)) == sizeof(r);
  f.close();
  return ok;
}

int memoirCount() {
  if (!sdReady) return 0;
  File f = SD_MMC.open(MEMOIR_PATH, FILE_READ);
  if (!f) return 0;
  int n = (int)(f.size() / sizeof(MemRec));
  f.close();
  return n;
}

bool memoirGet(int i, MemRec &r) {
  int n = memoirCount();
  if (i < 0 || i >= n) return false;
  File f = SD_MMC.open(MEMOIR_PATH, FILE_READ);
  if (!f) return false;
  bool ok = f.seek((uint32_t)(n - 1 - i) * sizeof(MemRec)) && f.read((uint8_t *)&r, sizeof(r)) == sizeof(r) &&
            r.ver == MEM_REC_VER && r.speciesId >= 1 && r.speciesId <= PET_DEX_MAX;
  f.close();
  return ok;
}
