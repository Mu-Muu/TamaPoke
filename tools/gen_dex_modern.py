#!/usr/bin/env python3
"""Genera tools/dex_data_modern.py (Pokedex 387-1025) desde los CSV de PokeAPI.

  python3 tools/gen_dex_modern.py

Las gen 1-3 (1-386) siguen siendo las de dex_data.py, curadas a mano. De aqui
salen las gen 4-9 en el mismo formato, mas lo que estas cambian en las viejas:
evoluciones nuevas de especies antiguas (Magneton -> Magnezone, Eevee ->
Leafeon...) y preevoluciones nuevas (Budew -> Roselia...).

Reglas, las mismas que ya usaba dex_data.py:
- evoluciones por nivel: su nivel real; por piedra/objeto ~30; por
  intercambio ~40; amistad, hora, movimiento y demas condiciones ~25.
- varias evoluciones desde la misma especie: la de numero mas bajo es la
  principal y el resto van a BRANCHES (el juego elige al azar).
- tipo: el primario de la forma por defecto.
- especies sin sprite animado en tools/sdcard/mons: no salen de huevo ni son
  destino de evolucion (SIN_SPRITE). Si falta una fase intermedia, la anterior
  evoluciona directamente a la siguiente (Pidove -> Unfezant).

Los CSV se bajan una vez a tools/downloads/pokeapi/ (ignorado por git).
"""
import csv
import os
import subprocess
import sys
import unicodedata

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from dex_data import DEX_GEN3 as DEX_OLD  # noqa: E402  (1-386, curado a mano)

CSV_DIR = os.path.join(HERE, 'downloads', 'pokeapi')
CSV_URL = 'https://raw.githubusercontent.com/PokeAPI/pokeapi/master/data/v2/csv/{}.csv'
MONS = os.path.join(HERE, 'sdcard', 'mons')
OUT = os.path.join(HERE, 'dex_data_modern.py')

FIRST, LAST = 387, 1025
OLD_MAX = FIRST - 1

TIPOS = {
    'normal': 'normal', 'fire': 'fuego', 'water': 'agua', 'grass': 'planta',
    'electric': 'electrico', 'ice': 'hielo', 'fighting': 'lucha',
    'poison': 'veneno', 'ground': 'tierra', 'flying': 'volador',
    'psychic': 'psiquico', 'bug': 'bicho', 'rock': 'roca', 'ghost': 'fantasma',
    'dragon': 'dragon', 'dark': 'siniestro', 'steel': 'acero', 'fairy': 'hada',
}
NIVEL_OBJETO = 30
NIVEL_INTERCAMBIO = 40
NIVEL_OTRO = 25
# una linea es rara si su forma final llega a esta suma de stats base
UMBRAL_RARO = 510


def tabla(nombre):
    path = os.path.join(CSV_DIR, f'{nombre}.csv')
    if not os.path.exists(path):
        os.makedirs(CSV_DIR, exist_ok=True)
        subprocess.run(['curl', '-s', '-L', '--max-time', '120', '-o', path,
                        CSV_URL.format(nombre)], check=True)
    with open(path, encoding='utf-8') as fh:
        return list(csv.DictReader(fh))


def ascii_up(s):
    s = s.replace('♀', 'F').replace('♂', 'M').replace('’', "'")  # Sirfetch’d
    s = unicodedata.normalize('NFD', s)
    s = ''.join(c for c in s if unicodedata.category(c) != 'Mn')
    return s.upper()


def nivel(fila, triggers):
    t = triggers[fila['evolution_trigger_id']]
    if t == 'level-up' and fila['minimum_level']:
        return int(fila['minimum_level'])
    if t == 'use-item':
        return NIVEL_OBJETO
    if t == 'trade':
        return NIVEL_INTERCAMBIO
    return NIVEL_OTRO


def main():
    species = {int(r['id']): r for r in tabla('pokemon_species') if int(r['id']) <= LAST}
    langs = {r['id']: r['identifier'] for r in tabla('languages')}
    en = {int(r['pokemon_species_id']): r['name'] for r in tabla('pokemon_species_names')
          if langs.get(r['local_language_id']) == 'en'}
    types = {r['id']: r['identifier'] for r in tabla('types')}
    tipo = {int(r['pokemon_id']): TIPOS[types[r['type_id']]] for r in tabla('pokemon_types')
            if r['slot'] == '1'}
    stats = {}
    for r in tabla('pokemon_stats'):
        stats.setdefault(int(r['pokemon_id']), {})[int(r['stat_id'])] = int(r['base_stat'])
    triggers = {r['id']: r['identifier'] for r in tabla('evolution_triggers')}
    evo_fila = {}  # especie -> primera fila de evolucion (la mas antigua)
    for r in tabla('pokemon_evolution'):
        n = int(r['evolved_species_id'])
        if n not in evo_fila or int(r['id']) < int(evo_fila[n]['id']):
            evo_fila[n] = r

    def con_sprite(n):
        return n <= OLD_MAX or os.path.exists(os.path.join(MONS, f'p{n:03d}.bin'))

    # aristas padre -> [(hijo, nivel)] de todo el arbol 1-1025. Los niveles
    # aproximados (objeto, amistad...) quedan al menos 5 por encima del nivel
    # al que se llega al padre: Magneton sale a 30, Magnezone no puede ser a 25.
    entrada_old = {d[4]: d[5] for d in DEX_OLD if d[4]}
    real = {}
    for n, r in species.items():
        if r['evolves_from_species_id']:
            f = evo_fila.get(n)
            real[n] = (nivel(f, triggers) if f else NIVEL_OTRO,
                       bool(f and triggers[f['evolution_trigger_id']] == 'level-up' and f['minimum_level']))

    def entrada(n, depth=0):
        if n <= OLD_MAX and n in entrada_old:
            return entrada_old[n]
        if n not in real or depth > 8:
            return 0
        lvl, exacto = real[n]
        padre = int(species[n]['evolves_from_species_id'])
        return lvl if exacto else max(lvl, entrada(padre, depth + 1) + 5)

    hijos = {}
    for n in real:
        hijos.setdefault(int(species[n]['evolves_from_species_id']), []).append((n, entrada(n)))

    def destinos(n):
        """Hijos jugables de n: salta las fases sin sprite (Pidove -> Unfezant)."""
        out = []
        for h, lvl in sorted(hijos.get(n, [])):
            if con_sprite(h):
                out.append((h, lvl))
            else:
                out += [(nieto, max(lvl, l2)) for nieto, l2 in destinos(h)]
        return out

    old_evo = {d[0]: d[4] for d in DEX_OLD}
    dex, branches, old_evos, sin_sprite = [], {}, [], set()
    for n in range(1, LAST + 1):
        ds = destinos(n)
        if n <= OLD_MAX:
            # especie vieja: solo lo que apunta a especies nuevas
            nuevos = [(h, lvl) for h, lvl in ds if h >= FIRST]
            for h, lvl in nuevos:
                if old_evo[n] == 0 and not any(o[0] == n for o in old_evos):
                    old_evos.append((n, h, lvl))
                else:
                    branches.setdefault(n, []).append(h)
            continue
        r = species[n]
        evo, lvl = (ds[0] if ds else (0, 0))
        for h, _ in ds[1:]:
            branches.setdefault(n, []).append(h)
        if not con_sprite(n):
            # sin sprite: no sale de huevo, asi que tampoco evoluciona. Si su
            # evolucion si tiene sprite (Frillish -> Jellicent) queda como base
            # y sale de huevo ella misma.
            sin_sprite.add(n)
            evo, lvl = 0, 0
            branches.pop(n, None)
        dex.append((n, r['identifier'], ascii_up(en[n]), tipo[n], evo, lvl))

    destino_de = {h for _, _, _, _, h, _ in dex if h} | {h for _, h, _ in old_evos}
    destino_de |= {h for v in branches.values() for h in v}
    destino_de |= {d[4] for d in DEX_OLD if d[4]}

    def bst(n):
        s = stats[n]
        return sum(s.values())

    def final_bst(n, depth=0):
        hs = [h for h, _ in destinos(n)]
        if not hs or depth > 6:
            return bst(n)
        return max(final_bst(h, depth + 1) for h in hs)

    legendary, rare = set(), set()
    for n, *_ in dex:
        if n in sin_sprite or n in destino_de:
            continue
        r = species[n]
        if r['is_legendary'] == '1' or r['is_mythical'] == '1':
            legendary.add(n)
        elif final_bst(n) >= UMBRAL_RARO:
            rare.add(n)

    base_stats = {n: (stats[n][1], stats[n][2], stats[n][3], stats[n][6]) for n, *_ in dex}

    with open(OUT, 'w', encoding='utf-8') as f:
        f.write('# -*- coding: utf-8 -*-\n')
        f.write('"""GENERADO por tools/gen_dex_modern.py desde PokeAPI - no editar a mano.\n\n')
        f.write('Pokedex 387-1025 en el formato de dex_data.py, mas lo que estas\n')
        f.write('generaciones cambian en las especies 1-386.\n"""\n\n')
        f.write('# num, slug, display, tipo, evolucionaA, nivel\n')
        f.write('DEX_MODERN = [\n')
        for row in dex:
            f.write(f'    {row!r},\n')
        f.write(']\n\n')
        f.write('# evoluciones nuevas de especies 1-386 que no evolucionaban: (de, a, nivel)\n')
        f.write(f'OLD_EVOS = {old_evos!r}\n\n')
        f.write('# destinos extra (se suman a los BRANCHES de dex_data.py)\n')
        f.write('BRANCHES_MODERN = {\n')
        for k in sorted(branches):
            f.write(f'    {k}: {branches[k]!r},\n')
        f.write('}\n\n')
        f.write(f'RARE_MODERN = {sorted(rare)!r}\n\n')
        f.write(f'LEGENDARY_MODERN = {sorted(legendary)!r}\n\n')
        f.write('# sin sprite animado (o inalcanzables): ni huevo ni evolucion\n')
        f.write(f'SIN_SPRITE = {sorted(sin_sprite)!r}\n\n')
        f.write('# stats base (hp, atk, def, vel)\n')
        f.write('STATS_MODERN = {\n')
        for k in sorted(base_stats):
            f.write(f'    {k}: {base_stats[k]!r},\n')
        f.write('}\n')
    print(f'guardado {os.path.normpath(OUT)}: {len(dex)} especies, {len(old_evos)} evoluciones '
          f'nuevas de 1-386, {sum(map(len, branches.values()))} ramas, {len(rare)} raras, '
          f'{len(legendary)} legendarias, {len(sin_sprite)} sin sprite')


if __name__ == '__main__':
    main()
