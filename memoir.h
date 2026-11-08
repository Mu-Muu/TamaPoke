#pragma once
#include "pet.h"

// Album de recuerdos: una pagina (MemRec) por cada compañero que acabo
// (despedida, escapada, liberado o a la granja), en la SD (/memoir.bin, una
// detras de otra). Sin SD no se guarda.
bool memoirAdd(const MemRec &r);
int memoirCount();
bool memoirGet(int i, MemRec &r);  // 0 = el mas reciente
