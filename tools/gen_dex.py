#!/usr/bin/env python3
"""Genera dex.h (tabla de la Pokedex para el firmware) desde dex_data.py.

  python3 tools/gen_dex.py
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from dex_data import DEX, TYPE_ACCENTS, CLASSIC, RARE, LEGENDARY, BRANCHES, SIN_SPRITE
from dex_stats import BASE_STATS
from dex_names import LOCAL_NAMES


def rgb565(hexcol):
    r, g, b = int(hexcol[1:3], 16), int(hexcol[3:5], 16), int(hexcol[5:7], 16)
    return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)


# bioma de fondo por tipo (la luz la pone la hora real del RTC)
# 0 PRADERA, 1 PLAYA, 2 BOSQUE, 3 VOLCAN, 4 MONTANA, 5 NIEVE
TYPE_BIOME = {
    'agua': 1, 'planta': 2, 'bicho': 2, 'fuego': 3,
    'roca': 4, 'tierra': 4, 'dragon': 1, 'hielo': 5,  # los dragones gen1 (Dratini) viven en el agua
    'normal': 0, 'electrico': 0, 'lucha': 0, 'veneno': 0,
    'psiquico': 0, 'fantasma': 0, 'siniestro': 0, 'acero': 4,
    'volador': 4, 'hada': 0,
}

# excepciones por dex# (el tipo no basta): fosiles marinos roca/agua -> playa
BIOME_OVERRIDE = {138: 1, 139: 1, 140: 1, 141: 1}  # Omanyte, Omastar, Kabuto, Kabutops


def main():
    out = []
    out.append("#pragma once\n#include <stdint.h>\n#include \"i18n.h\"  // gLang\n\n")
    out.append("// GENERADO por tools/gen_dex.py desde tools/dex_data.py - no editar\n\n")
    out.append(f"#define DEX_COUNT {len(DEX)}\n")
    out.append(f"#define DEX_BITMAP_BYTES {(len(DEX) + 7) // 8}  // pokedex en bits (dexReg)\n\n")
    out.append(
        "// rareza: 0 = solo por evolucion, 1 = comun, 2 = raro, 3 = legendario,\n"
        "// 4 = no disponible (sin sprite animado: ni huevo ni evolucion)\n"
        "enum : uint8_t { R_EVO = 0, R_COMUN, R_RARO, R_LEGENDARIO, R_NONE };\n\n"
        "struct DexEntry {\n"
        "  const char *name;\n"
        "  uint16_t evolvesTo;   // numero de dex, 0 = forma final\n"
        "  uint8_t evolveLevel;\n"
        "  uint8_t rarity;       // sale de huevo si es R_COMUN..R_LEGENDARIO\n"
        "  uint16_t accent;      // color RGB565 del tipo para la UI\n"
        "  uint8_t bHp, bAtk, bDef, bSpe;  // base stats reales de gen 1\n"
        "  uint8_t biome;        // 0 pradera 1 playa 2 bosque 3 volcan 4 montana 5 nieve\n"
        "};\n\n")
    # formas base = las que no son evolucion de nadie (las ramas tambien lo son)
    evolved = {d[4] for d in DEX if d[4]} | {t for ts in BRANCHES.values() for t in ts}
    rarities = []
    out.append("static const DexEntry DEX_TBL[DEX_COUNT + 1] = {\n")
    out.append('  { "?", 0, 0, 0, 0x2946, 50, 50, 50, 50, 0 },  // 0: sin usar\n')
    for num, slug, display, typ, evo, lvl in DEX:
        acc = rgb565(TYPE_ACCENTS[typ])
        if num in SIN_SPRITE:
            rar = 'R_NONE'
        elif num in evolved:
            rar = 'R_EVO'
        elif num in LEGENDARY:
            rar = 'R_LEGENDARIO'
        elif num in RARE:
            rar = 'R_RARO'
        else:
            rar = 'R_COMUN'
        rarities.append(rar)
        hp, atk, df, spe = BASE_STATS[num]
        bio = BIOME_OVERRIDE.get(num, TYPE_BIOME[typ])
        out.append(f'  {{ "{display}", {evo}, {lvl}, {rar}, 0x{acc:04X}, {hp}, {atk}, {df}, {spe}, {bio} }},  // {num} {typ}\n')
    out.append("};\n\n")

    # El ingles es el de DEX_TBL; el chino va aparte, en UTF-8.
    out.append(
        "// Nombres oficiales en chino simplificado, en UTF-8 (se pintan con la\n"
        "// fuente U8g2). nullptr = sin nombre propio.\n")
    for lg in ('zh',):
        out.append(f"static const char *const DEX_NAME_{lg.upper()}[DEX_COUNT + 1] = {{\n")
        fila = []
        for num in range(0, len(DEX) + 1):
            nm = LOCAL_NAMES.get(num, {}).get(lg)
            fila.append(f'"{nm}"' if nm else 'nullptr')
            if len(fila) == 4:
                out.append("  " + ", ".join(fila) + ",\n")
                fila = []
        if fila:
            out.append("  " + ", ".join(fila) + ",\n")
        out.append("};\n\n")
    out.append(
        "// Nombre de la especie en el idioma activo (cae al de DEX_TBL si ese\n"
        "// idioma no tiene nombre propio para ella).\n"
        "static inline const char *dexName(int16_t dex) {\n"
        "  if (dex < 1 || dex > DEX_COUNT) return DEX_TBL[0].name;\n"
        "  const char *n = (gLang == LANG_ZH) ? DEX_NAME_ZH[dex] : nullptr;\n"
        "  return n ? n : DEX_TBL[dex].name;\n"
        "}\n\n")

    out.append("// destinos extra de las especies que se ramifican (ademas de evolvesTo);\n"
               "// Pet::evolve() elige al azar entre todos\n")
    out.append("struct DexBranch { uint16_t from, to; };\n")
    out.append("static const DexBranch DEX_BRANCHES[] = {\n")
    for frm in sorted(BRANCHES):
        out.append("  " + " ".join(f"{{ {frm}, {to} }}," for to in BRANCHES[frm]) + "\n")
    out.append("};\n")
    out.append(f"#define NUM_DEX_BRANCHES {sum(len(v) for v in BRANCHES.values())}\n\n")

    out.append("// el primer huevo de la partida: iniciales clasicos\n")
    out.append("static const int16_t CLASSIC_DEX[] = { %s };\n" % ", ".join(map(str, CLASSIC)))
    out.append(f"#define NUM_CLASSIC_DEX {len(CLASSIC)}\n")
    from collections import Counter
    c = Counter(rarities)
    print(f"bases: {c['R_COMUN']} comunes, {c['R_RARO']} raras, {c['R_LEGENDARIO']} legendarias, "
          f"{c['R_EVO']} solo-evolucion, {c['R_NONE']} sin sprite")

    path = os.path.join(os.path.dirname(__file__), '..', 'dex.h')
    open(path, 'w').write(''.join(out))
    print(f"guardado {os.path.normpath(path)} ({len(DEX)} especies)")


if __name__ == '__main__':
    main()
