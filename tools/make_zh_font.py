#!/usr/bin/env python3
"""Fuente china recortada para el firmware (formato de fuente U8g2).

Las u8g2_font_wqy*_t_gb2312 traen GB2312 entero (~7500 glifos por tamaño) y aun
asi les falta algun caracter de los nombres oficiales (椪, de Ogerpon). Este
script saca de las WenQuanYi bitmap (las mismas BDF de las que U8g2 genera las
wqy: wenquanyi_12pt -> 16 px, wenquanyi_9pt -> 12 px) solo los caracteres que
se pintan: los textos chinos de i18n.cpp y los nombres de dex_names.py, mas el
ASCII. Escribe zhfont.h (u8g2_font_tp_zh16 y u8g2_font_tp_zh12) y
tools/zh_font_chars.txt (lo que lleva, para el test).

    python3 tools/make_zh_font.py <dir con wenquanyi_9pt.bdf y wenquanyi_12pt.bdf>

Las BDF estan en el repositorio de U8g2: tools/font/bdf/wenquanyi_*.bdf.
El codificador sigue el formato que lee Arduino_GFX (copia del de U8g2):
cabecera de 23 bytes, glifos < 256, tabla de busqueda unicode y glifos
unicode; cada glifo con ancho/alto/x/y/avance y los pixeles en RLE
(ceros, unos, bit de repeticion), bits empaquetados del menos significativo
al mas.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

FONTS = [('u8g2_font_tp_zh16', 'wenquanyi_12pt.bdf'), ('u8g2_font_tp_zh12', 'wenquanyi_9pt.bdf')]


# ---------------------------------------------------------------- BDF
def parse_bdf(path):
    """{codigo: (w, h, xoff, yoff, dx, filas)}; filas = lista de enteros (bit alto = izquierda)."""
    glyphs = {}
    enc = dx = None
    bbx = None
    rows = None
    with open(path, encoding='latin-1') as fh:
        for line in fh:
            p = line.split()
            if not p:
                continue
            if p[0] == 'STARTCHAR':
                enc, dx, bbx, rows = None, 0, None, None
            elif p[0] == 'ENCODING':
                enc = int(p[1])
            elif p[0] == 'DWIDTH':
                dx = int(p[1])
            elif p[0] == 'BBX':
                bbx = tuple(int(v) for v in p[1:5])
            elif p[0] == 'BITMAP':
                rows = []
            elif p[0] == 'ENDCHAR':
                if enc is not None and enc >= 0 and bbx:
                    w, h, xo, yo = bbx
                    nb = (w + 7) // 8 * 8
                    filas = [int(r, 16) >> (len(r) * 4 - nb) if len(r) * 4 >= nb else int(r, 16) << (nb - len(r) * 4)
                             for r in rows]
                    glyphs[enc] = (w, h, xo, yo, dx, [f >> (nb - w) for f in filas])
                rows = None
            elif rows is not None:
                rows.append(p[0])
    return glyphs


def trim(g):
    """Recorta filas y columnas vacias (ajustando x/y), como hace bdfconv."""
    w, h, xo, yo, dx, filas = g
    pix = [[(filas[r] >> (w - 1 - c)) & 1 for c in range(w)] for r in range(h)]
    ys = [r for r in range(h) if any(pix[r])]
    xs = [c for c in range(w) if any(pix[r][c] for r in range(h))]
    if not ys:
        return (0, 0, 0, 0, dx, [])
    r0, r1, c0, c1 = ys[0], ys[-1], xs[0], xs[-1]
    nw, nh = c1 - c0 + 1, r1 - r0 + 1
    sub = [row[c0:c1 + 1] for row in pix[r0:r1 + 1]]
    # la fila de abajo del BBX esta en yo; quitar filas por abajo sube el origen
    return (nw, nh, xo + c0, yo + (h - 1 - r1), dx, sub)


# ---------------------------------------------------------------- codificador U8g2
class Bits:
    def __init__(self):
        self.out = bytearray()
        self.acc = 0
        self.n = 0

    def put(self, val, cnt):
        for i in range(cnt):  # del menos significativo al mas
            self.acc |= ((val >> i) & 1) << self.n
            self.n += 1
            if self.n == 8:
                self.out.append(self.acc)
                self.acc = self.n = 0

    def bytes(self):
        return bytes(self.out) + (bytes([self.acc]) if self.n else b'')


def unsigned_bits(vals):
    m = max(vals) if vals else 0
    n = 1
    while m >= (1 << n):
        n += 1
    return n


def signed_bits(vals):
    n = 1
    while any(v < -(1 << (n - 1)) or v > (1 << (n - 1)) - 1 for v in vals):
        n += 1
    return n


def runs(pix):
    """Pares (ceros, unos) recorriendo filas de arriba abajo, de izquierda a derecha."""
    flat = [v for row in pix for v in row]
    out, i, n = [], 0, len(flat)
    while i < n:
        a = 0
        while i < n and flat[i] == 0:
            a, i = a + 1, i + 1
        b = 0
        while i < n and flat[i] == 1:
            b, i = b + 1, i + 1
        out.append((a, b))
    return out


def rle_pairs(pairs, m0, m1):
    """Parte los tramos demasiado largos para m0/m1 bits."""
    max0, max1 = (1 << m0) - 1, (1 << m1) - 1
    res = []
    for a, b in pairs:
        while a > max0:
            res.append((max0, 0))
            a -= max0
        while b > max1:
            res.append((a, max1))
            a, b = 0, b - max1
        res.append((a, b))
    return res


def encode_glyph(g, hdr, m0, m1):
    w, h, xo, yo, dx, pix = g
    bits = Bits()
    bits.put(w, hdr['bw'])
    bits.put(h, hdr['bh'])
    bits.put(xo + (1 << (hdr['bx'] - 1)), hdr['bx'])
    bits.put(yo + (1 << (hdr['by'] - 1)), hdr['by'])
    bits.put(dx + (1 << (hdr['bd'] - 1)), hdr['bd'])
    if w and h:
        pares = rle_pairs(runs(pix), m0, m1)
        i = 0
        while i < len(pares):
            a, b = pares[i]
            bits.put(a, m0)
            bits.put(b, m1)
            i += 1
            while i < len(pares) and pares[i] == (a, b):  # repeticion
                bits.put(1, 1)
                i += 1
            bits.put(0, 1)
    return bits.bytes()


def build_font(glyphs):
    """glyphs: {codigo: glifo recortado}. Devuelve los bytes de la fuente."""
    codes = sorted(glyphs)
    gs = [glyphs[c] for c in codes]
    hdr = {
        'bw': unsigned_bits([g[0] for g in gs]),
        'bh': unsigned_bits([g[1] for g in gs]),
        'bx': signed_bits([g[2] for g in gs]),
        'by': signed_bits([g[3] for g in gs]),
        'bd': signed_bits([g[4] for g in gs]),
    }
    best = None
    for m0 in range(2, 9):
        for m1 in range(2, 9):
            total = sum(len(encode_glyph(g, hdr, m0, m1)) for g in gs)
            if best is None or total < best[0]:
                best = (total, m0, m1)
    _, m0, m1 = best

    low = bytearray()
    pos_A = pos_a = None
    for c in [c for c in codes if c < 256]:
        if pos_A is None and c >= ord('A'):
            pos_A = len(low)
        if pos_a is None and c >= ord('a'):
            pos_a = len(low)
        data = encode_glyph(glyphs[c], hdr, m0, m1)
        assert len(data) + 2 <= 255, c
        low += bytes([c, len(data) + 2]) + data
    if pos_A is None:
        pos_A = len(low)
    if pos_a is None:
        pos_a = len(low)
    low += b'\x00\x00'

    # glifos unicode en grupos, con su tabla de busqueda
    uni = [c for c in codes if c >= 256]
    grupos, actual = [], []
    for c in uni:
        data = encode_glyph(glyphs[c], hdr, m0, m1)
        assert len(data) + 3 <= 255, c
        actual.append(bytes([c >> 8, c & 0xFF, len(data) + 3]) + data)
        if len(actual) == 64:
            grupos.append((c, b''.join(actual)))
            actual = []
    if actual:
        grupos.append((uni[-1], b''.join(actual)))
    tabla = bytearray()
    n_ent = len(grupos) + 1
    prev = None
    for i, (ultimo, blob) in enumerate(grupos):
        delta = 4 * n_ent if i == 0 else len(grupos[i - 1][1])
        tabla += bytes([delta >> 8, delta & 0xFF, ultimo >> 8, ultimo & 0xFF])
        prev = blob
    delta = len(prev) if prev is not None else 4 * n_ent
    tabla += bytes([delta >> 8, delta & 0xFF, 0xFF, 0xFF])
    uni_blob = bytes(tabla) + b''.join(b for _, b in grupos) + b'\x00\x00'

    def info(ch, top):
        g = glyphs.get(ord(ch))
        if not g:
            return 0
        return (g[1] + g[3]) if top else g[3]

    head = bytes([
        min(len(codes), 255), 0, m0, m1, hdr['bw'], hdr['bh'], hdr['bx'], hdr['by'], hdr['bd'],
        max(g[0] for g in gs), max(g[1] for g in gs),
        min(g[2] for g in gs) & 0xFF, min(g[3] for g in gs) & 0xFF,
        info('A', True) & 0xFF, info('g', False) & 0xFF, info('(', True) & 0xFF, info(')', False) & 0xFF,
        pos_A >> 8, pos_A & 0xFF, pos_a >> 8, pos_a & 0xFF, len(low) >> 8, len(low) & 0xFF,
    ])
    return head + bytes(low) + uni_blob


# ---------------------------------------------------------------- decodificador (para comprobar)
def decode_font(font, code):
    """Como Arduino_GFX: (w, h, x, y, dx, pixeles) del glifo, o None."""
    m0, m1, bw, bh, bx, by, bd = font[2], font[3], font[4], font[5], font[6], font[7], font[8]
    word = lambda p: (font[p] << 8) | font[p + 1]  # noqa: E731
    base = 23
    data = None
    if code < 256:
        p = base + (word(19) if code >= ord('a') else word(17) if code >= ord('A') else 0)
        while font[p + 1]:
            if font[p] == code:
                data = p + 2
                break
            p += font[p + 1]
    else:
        p = base + word(21)
        t = p
        while True:
            p += word(t)
            e = word(t + 2)
            t += 4
            if e >= code:
                break
        while True:
            e = word(p)
            if e == 0:
                break
            if e == code:
                data = p + 3
                break
            p += font[p + 2]
    if data is None:
        return None
    st = {'p': data, 'b': 0}

    def get(cnt):
        val = 0
        for i in range(cnt):
            val |= ((font[st['p']] >> st['b']) & 1) << i
            st['b'] += 1
            if st['b'] == 8:
                st['b'] = 0
                st['p'] += 1
        return val

    def sget(cnt):
        return get(cnt) - (1 << (cnt - 1))

    w, h, x, y, dx = get(bw), get(bh), sget(bx), sget(by), sget(bd)
    pix = [[0] * w for _ in range(h)]
    if w:
        cx = cy = 0

        def run(n, v):
            nonlocal cx, cy
            for _ in range(n):
                if cy < h:
                    pix[cy][cx] = v
                cx += 1
                if cx == w:
                    cx, cy = 0, cy + 1

        while True:
            a, b = get(m0), get(m1)
            while True:
                run(a, 0)
                run(b, 1)
                if not get(1):
                    break
            if cy >= h:
                break
    return (w, h, x, y, dx, pix)


# ---------------------------------------------------------------- caracteres usados
def used_chars():
    import test_i18n_formats as fmt
    from dex_names import LOCAL_NAMES

    with open(os.path.join(ROOT, 'i18n.cpp'), encoding='utf-8') as fh:
        src = fh.read()
    zh = fmt.LANGS.index('ZH')
    textos = []
    for tabla in ('STRINGS', 'MED_NAME', 'MED_LBL', 'MED_DSC'):
        textos += [lit[1:-1] for lit in fmt.extract_table(src, tabla, len(fmt.LANGS))[zh]]
    textos += [n['zh'] for n in LOCAL_NAMES.values() if 'zh' in n]
    return sorted({ord(c) for t in textos for c in t if ord(c) > 126})


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    bdf_dir = sys.argv[1]
    usados = used_chars()
    codes = list(range(32, 127)) + usados
    salida = ['#pragma once', '#include <stdint.h>', '',
              '// GENERADO por tools/make_zh_font.py desde las WenQuanYi bitmap de U8g2 - no editar.',
              f'// {len(usados)} caracteres no ASCII (lista en tools/zh_font_chars.txt) + ASCII 32-126.', '']
    for nombre, bdf in FONTS:
        todos = parse_bdf(os.path.join(bdf_dir, bdf))
        faltan = [chr(c) for c in codes if c not in todos]
        if faltan:
            sys.exit(f'{bdf}: no tiene {"".join(faltan)}')
        sel = {c: trim(todos[c]) for c in codes}
        font = build_font(sel)
        for c in codes:  # cada glifo se lee tal cual se escribio
            assert decode_font(font, c) == sel[c], (bdf, chr(c))
        salida.append(f'// {bdf}: {len(font)} bytes')
        salida.append(f'static const uint8_t {nombre}[{len(font)}] = {{')
        for i in range(0, len(font), 24):
            salida.append('  ' + ', '.join(f'0x{b:02x}' for b in font[i:i + 24]) + ',')
        salida.append('};')
        salida.append('')
        print(f'{nombre}: {len(font)} bytes')
    with open(os.path.join(ROOT, 'zhfont.h'), 'w', encoding='utf-8') as fh:
        fh.write('\n'.join(salida))
    with open(os.path.join(HERE, 'zh_font_chars.txt'), 'w', encoding='utf-8') as fh:
        fh.write(''.join(chr(c) for c in usados) + '\n')
    print(f'{len(usados)} caracteres')


if __name__ == '__main__':
    main()
