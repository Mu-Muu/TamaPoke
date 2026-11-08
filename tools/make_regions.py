#!/usr/bin/env python3
"""Genera regions.h (las 9 regiones con su rango de Pokedex y una imagen
pequena para el menu de la Pokedex) y los fondos de la pantalla principal,
tools/sdcard/mons/bgN.bin (N = 1-9), que van a la SD.

  python3 tools/make_regions.py

La imagen sale de tools/assets/regions_source.png (la lamina de los 9 paisajes,
3x3, aportada por el usuario). La casilla del menu se recorta por debajo de la
etiqueta "第N代". El fondo se recorta a la medida de la escena de la pantalla
(BG_W x BG_H, encima del panel inferior) evitando esa etiqueta, y su franja de
arriba se aclara para que se lea la cabecera (nombre y estado) encima.

Formato TPBG (little-endian): char[4] "TPBG", u16 w, u16 h, u16 px[w*h] RGB565.
"""
import os
import struct

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, 'assets', 'regions_source.png')
OUT = os.path.join(HERE, '..', 'regions.h')

W, H = 116, 46  # casilla del menu de regiones
BG_W, BG_H = 466, 312  # fondo: la escena por encima del panel inferior (y 312)
BG_DIR = os.path.join(HERE, 'sdcard', 'mons')

# (primero, ultimo) numero de Pokedex de cada generacion
RANGOS = [(1, 151), (152, 251), (252, 386), (387, 493), (494, 649),
          (650, 721), (722, 809), (810, 905), (906, 1025)]
# escenas en la lamina: columnas x y filas y (sin la etiqueta "第N代" ni el pie)
COLS = [(14, 511), (524, 1021), (1030, 1525)]
FILAS = [(156, 311), (430, 585), (706, 865)]
# escena completa, con la franja de la etiqueta (para el fondo)
FILAS_FONDO = [(118, 311), (392, 585), (668, 865)]
# recorte del fondo por region: 'A' = altura completa pegado a la derecha (la
# etiqueta queda fuera, a la izquierda); 'B' = por debajo de la etiqueta y
# centrado. B en Johto, Hoenn y Sinnoh para no tener a Ho-Oh, Rayquaza o
# Palkia enormes detras del compañero.
RECORTE_FONDO = ['A', 'B', 'B', 'B', 'A', 'A', 'A', 'A', 'A']
ACLARADO_H = 120     # franja de arriba que se aclara (cabecera)
ACLARADO_MAX = 0.45  # cuanto se aclara la primera fila (baja a 0 en ACLARADO_H)


def rgb565(r, g, b):
    return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)


def escena(im, i):
    (x0, x1), (y0, y1) = COLS[i % 3], FILAS[i // 3]
    # centra al formato de la casilla recortando los lados
    ancho = round((y1 - y0) * W / H)
    cx = (x0 + x1) // 2
    return im.crop((cx - ancho // 2, y0, cx + ancho // 2, y1)).resize((W, H), Image.LANCZOS)


def fondo(im, i):
    (x0, x1), (y0, y1) = COLS[i % 3], FILAS_FONDO[i // 3]
    if RECORTE_FONDO[i] == 'A':
        h = y1 - y0
        w = round(h * BG_W / BG_H)
        caja = (x1 - w, y0, x1, y1)
    else:
        top = y0 + 40
        h = y1 - top
        w = round(h * BG_W / BG_H)
        cx = (x0 + x1) // 2
        caja = (cx - w // 2, top, cx + w // 2, y1)
    img = im.crop(caja).resize((BG_W, BG_H), Image.LANCZOS)
    blanco = Image.new('RGB', (BG_W, 1), (255, 255, 255))
    for y in range(ACLARADO_H):
        a = ACLARADO_MAX * (1 - y / ACLARADO_H)
        fila = img.crop((0, y, BG_W, y + 1))
        img.paste(Image.blend(fila, blanco, a), (0, y))
    return img


def escribe_fondo(img, path):
    raw = img.tobytes()
    px = [rgb565(raw[k], raw[k + 1], raw[k + 2]) for k in range(0, len(raw), 3)]
    with open(path, 'wb') as f:
        f.write(b'TPBG')
        f.write(struct.pack('<HH', BG_W, BG_H))
        f.write(struct.pack(f'<{len(px)}H', *px))


def main():
    im = Image.open(SRC).convert('RGB')
    os.makedirs(BG_DIR, exist_ok=True)
    for i in range(len(RANGOS)):
        escribe_fondo(fondo(im, i), os.path.join(BG_DIR, f'bg{i + 1}.bin'))
    out = ['#pragma once\n#include <stdint.h>\n\n',
           '// GENERADO por tools/make_regions.py desde tools/assets/regions_source.png - no editar\n\n',
           f'#define REGION_COUNT {len(RANGOS)}\n',
           f'#define REGION_IMG_W {W}\n#define REGION_IMG_H {H}\n\n',
           'struct Region { int16_t first, last; };\n',
           'static const Region REGIONS[REGION_COUNT] = {\n']
    out += [f'  {{ {a}, {b} }},\n' for a, b in RANGOS]
    out.append('};\n\n')
    out.append('// paisaje de cada region, RGB565\n')
    out.append('static const uint16_t REGION_IMG[REGION_COUNT][REGION_IMG_W * REGION_IMG_H] = {\n')
    for i in range(len(RANGOS)):
        raw = escena(im, i).tobytes()
        px = [rgb565(raw[k], raw[k + 1], raw[k + 2]) for k in range(0, len(raw), 3)]
        out.append('  {\n')
        for k in range(0, len(px), 16):
            out.append('    ' + ', '.join(f'0x{v:04X}' for v in px[k:k + 16]) + ',\n')
        out.append('  },\n')
    out.append('};\n')
    with open(OUT, 'w') as f:
        f.write(''.join(out))
    print(f'guardado {os.path.normpath(OUT)}: {len(RANGOS)} regiones de {W}x{H}, '
          f'y {len(RANGOS)} fondos {BG_W}x{BG_H} en {os.path.normpath(BG_DIR)}')


if __name__ == '__main__':
    main()
