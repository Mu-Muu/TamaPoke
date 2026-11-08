#!/usr/bin/env python3
"""Saca de PokeAPI los nombres oficiales en chino y escribe tools/dex_names.py.

  python3 tools/gen_names.py

El ingles sale de dex_data.py; aqui solo se guarda el chino simplificado, que va
en UTF-8 tal cual porque se pinta con una fuente U8g2. Usa los CSV de PokeAPI
(los mismos que gen_dex_modern.py, en tools/downloads/pokeapi/).
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_dex_modern import tabla  # noqa: E402

DEX_MAX = 1025  # ultima especie (gen 9)

# Erratas de PokeAPI que se corrigen al generar: caracteres tradicionales en
# zh-hans, que ademas no estan en GB2312 (la fuente china).
CORRECCIONES = {
    (312, 'zh'): '负电拍拍',  # Minun: 負
    (973, 'zh'): '缠红鹤',    # Flamigo: 纏
    (990, 'zh'): '铁辙迹',    # Iron Treads: 轍
}


def main():
    langs = {r['id']: r['identifier'] for r in tabla('languages')}
    out = {}
    for r in tabla('pokemon_species_names'):
        num = int(r['pokemon_species_id'])
        if num <= DEX_MAX and langs.get(r['local_language_id']) == 'zh-hans':
            out[num] = {'zh': CORRECCIONES.get((num, 'zh'), r['name'])}

    path = os.path.join(os.path.dirname(__file__), 'dex_names.py')
    with open(path, 'w', encoding='utf-8') as f:
        f.write('# -*- coding: utf-8 -*-\n')
        f.write('"""GENERADO por tools/gen_names.py desde PokeAPI - no editar a mano.\n\n')
        f.write('Nombres oficiales en chino simplificado, en UTF-8 tal cual, porque se\n')
        f.write('pintan con una fuente U8g2.\n"""\n\n')
        f.write('LOCAL_NAMES = {\n')
        for num in sorted(out):
            f.write(f'    {num}: {out[num]!r},\n')
        f.write('}\n')
    print(f'guardado {os.path.normpath(path)}: {len(out)} especies con nombre propio')


if __name__ == '__main__':
    main()
