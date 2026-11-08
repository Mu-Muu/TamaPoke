#!/usr/bin/env python3
"""Tests de las herramientas del taller (tools/) y de los ficheros generados.

  python3 test/test_tools.py
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.path.join(ROOT, 'tools')
sys.path.insert(0, TOOLS)


class TestToolsCompile(unittest.TestCase):
    def test_todos_los_scripts_compilan(self):
        scripts = sorted(f for f in os.listdir(TOOLS) if f.endswith('.py'))
        self.assertTrue(scripts, 'no se encontro ningun script en tools/')
        for name in scripts:
            with self.subTest(script=name):
                with open(os.path.join(TOOLS, name), encoding='utf-8') as fh:
                    src = fh.read()
                try:
                    compile(src, name, 'exec')
                except SyntaxError as e:
                    self.fail(f'{name} no compila: {e}')

    def test_los_scripts_ejecutables_declaran_shebang(self):
        for name in sorted(f for f in os.listdir(TOOLS) if f.endswith('.py')):
            path = os.path.join(TOOLS, name)
            if not os.access(path, os.X_OK):
                continue
            with open(path) as fh:
                self.assertTrue(fh.readline().startswith('#!'),
                                f'{name} es ejecutable pero no tiene shebang')


class TestDexData(unittest.TestCase):
    """Invariantes de la fuente de datos de la Pokedex."""

    @classmethod
    def setUpClass(cls):
        from dex_data import DEX, TYPE_ACCENTS, CLASSIC, RARE, LEGENDARY, SLUGS
        from dex_stats import BASE_STATS
        from dex_names import LOCAL_NAMES
        cls.DEX, cls.ACCENTS, cls.CLASSIC = DEX, TYPE_ACCENTS, CLASSIC
        cls.RARE, cls.LEGENDARY, cls.SLUGS = RARE, LEGENDARY, SLUGS
        cls.STATS, cls.NAMES = BASE_STATS, LOCAL_NAMES
        cls.byNum = {row[0]: row for row in DEX}

    def test_estan_las_1025_especies_sin_huecos_ni_repetidos(self):
        nums = [row[0] for row in self.DEX]
        self.assertEqual(len(nums), 1025)
        self.assertEqual(sorted(nums), list(range(1, 1026)))

    def test_los_slugs_son_unicos_y_en_minusculas(self):
        slugs = [row[1] for row in self.DEX]
        self.assertEqual(len(set(slugs)), len(slugs), 'hay slugs repetidos')
        for s in slugs:
            self.assertEqual(s, s.lower(), f'slug con mayusculas: {s}')
            self.assertTrue(s.isascii(), f'slug no ASCII: {s}')

    def test_los_nombres_de_pantalla_son_ascii_en_mayusculas(self):
        # la fuente GFX del firmware no tiene acentos ni minusculas
        for num, slug, display, *_ in self.DEX:
            self.assertTrue(display.isascii(), f'{num} {display} no es ASCII')
            self.assertEqual(display, display.upper(), f'{num} {display} lleva minusculas')
            self.assertLessEqual(len(display), 12, f'{num} {display} no cabe en pantalla')

    def test_las_evoluciones_apuntan_a_especies_reales(self):
        for num, slug, display, tipo, evo, lvl in self.DEX:
            if evo:
                self.assertIn(evo, self.byNum, f'{display} evoluciona a {evo}, que no existe')
                self.assertNotEqual(evo, num, f'{display} evoluciona a si mismo')
                self.assertGreater(lvl, 0, f'{display} evoluciona sin nivel')
            else:
                self.assertEqual(lvl, 0, f'{display} es forma final pero tiene nivel {lvl}')

    def test_ninguna_cadena_evolutiva_hace_bucle(self):
        for num in self.byNum:
            seen, cur = set(), num
            while cur and self.byNum[cur][4]:
                self.assertNotIn(cur, seen, f'bucle evolutivo desde {num}')
                seen.add(cur)
                cur = self.byNum[cur][4]
                self.assertLessEqual(len(seen), 5, f'cadena demasiado larga desde {num}')

    def test_todos_los_tipos_tienen_color(self):
        for num, slug, display, tipo, *_ in self.DEX:
            self.assertIn(tipo, self.ACCENTS, f'{display}: tipo "{tipo}" sin color')

    def test_los_colores_son_hex_de_6_digitos(self):
        for tipo, col in self.ACCENTS.items():
            self.assertRegex(col, r'^#[0-9a-fA-F]{6}$', f'color raro en {tipo}: {col}')

    def test_las_rarezas_apuntan_a_formas_base(self):
        evolucionadas = {row[4] for row in self.DEX if row[4]}
        for num in sorted(self.RARE | self.LEGENDARY):
            self.assertIn(num, self.byNum, f'rareza para una especie inexistente: {num}')
            self.assertNotIn(num, evolucionadas,
                             f'{self.byNum[num][2]} solo sale por evolucion: la rareza no le sirve')
        self.assertFalse(self.RARE & self.LEGENDARY, 'una especie no puede ser rara y legendaria')

    def test_los_iniciales_clasicos_existen(self):
        from dex_data import BRANCHES
        evolucionadas = {row[4] for row in self.DEX if row[4]} | {t for v in BRANCHES.values() for t in v}
        for num in self.CLASSIC:
            self.assertIn(num, self.byNum, f'inicial inexistente: {num}')
            self.assertNotIn(num, evolucionadas, f'{self.byNum[num][2]} no puede ser inicial')

    def test_hay_stats_base_de_todas(self):
        for num in self.byNum:
            self.assertIn(num, self.STATS, f'faltan stats de {self.byNum[num][2]}')
            hp, atk, dfn, spe = self.STATS[num]
            for v in (hp, atk, dfn, spe):
                self.assertTrue(0 < v <= 255, f'stat fuera de rango en {self.byNum[num][2]}: {v}')

    def test_los_nombres_traducidos_cuadran_con_su_fuente(self):
        # Solo hay nombres chinos: van en UTF-8 con una fuente U8g2, asi que el
        # limite util son CARACTERES, no bytes (cada hanzi ocupa 3).
        for num, langs in self.NAMES.items():
            self.assertIn(num, self.byNum, f'nombre traducido de una especie inexistente: {num}')
            for lang, name in langs.items():
                self.assertEqual(lang, 'zh', f'idioma inesperado: {lang}')
                self.assertTrue(name, f'{num} {lang} vacio')
                self.assertLessEqual(len(name), 8, f'{num} {lang} no cabe en pantalla: {name}')


class TestGeneratedFiles(unittest.TestCase):
    """dex.h esta generado: si no coincide con su generador, alguien lo edito a mano."""

    def test_gen_dex_reproduce_dex_h(self):
        tmp = tempfile.mkdtemp(prefix='tamapoke-gen-')
        try:
            shutil.copytree(TOOLS, os.path.join(tmp, 'tools'))
            r = subprocess.run([sys.executable, os.path.join(tmp, 'tools', 'gen_dex.py')],
                               capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, f'gen_dex.py fallo:\n{r.stderr}')
            generated = os.path.join(tmp, 'dex.h')
            self.assertTrue(os.path.exists(generated), 'gen_dex.py no escribio dex.h')
            with open(generated) as a, open(os.path.join(ROOT, 'dex.h')) as b:
                self.assertEqual(a.read(), b.read(),
                                 'dex.h no coincide con lo que genera tools/gen_dex.py '
                                 '(vuelve a generarlo en vez de editarlo a mano)')
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    def test_los_ficheros_generados_avisan_de_que_lo_son(self):
        for name in ('dex.h', 'species.h', 'zhfont.h'):
            with open(os.path.join(ROOT, name)) as fh:
                head = fh.read(400)
            self.assertIn('GENERADO', head, f'{name} deberia decir que esta generado')


class TestSketchSanity(unittest.TestCase):
    """Comprobaciones baratas sobre el .ino que no necesitan compilarlo."""

    @classmethod
    def setUpClass(cls):
        # el sketch esta repartido en pestañas: TamaPoke.ino y las demas .ino
        cls.ino = {}
        for nombre in sorted(f for f in os.listdir(ROOT) if f.endswith('.ino')):
            with open(os.path.join(ROOT, nombre), encoding='utf-8') as fh:
                cls.ino[nombre] = fh.read()
        assert 'TamaPoke.ino' in cls.ino

    def test_ningun_numero_de_pokedex_cabe_en_8_bits(self):
        # Desde gen 2-3 hay especies por encima de 255: un uint8_t las trunca
        # en silencio (la 257 cargaba el sprite de la 1). Ni parametros ni
        # variables de 8 bits con nombre de especie en el firmware.
        # (los arrays como dexReg[] son bitmaps de bytes: esos si)
        patron = re.compile(r'\buint8_t[\s*&]+(dex\w*|species\w*|eggTarget|evolvesTo|galleryDetail)\b(?!\s*\[)')
        fuentes = [f for f in os.listdir(ROOT) if f.endswith(('.ino', '.cpp', '.h'))]
        for nombre in fuentes:
            with open(os.path.join(ROOT, nombre), encoding='utf-8') as fh:
                for lineno, line in enumerate(fh, 1):
                    code = line.split('//')[0]
                    with self.subTest(fichero=nombre, linea=lineno):
                        self.assertIsNone(patron.search(code), f'{nombre}:{lineno}: {code.strip()}')

    def test_el_sketch_no_lleva_caracteres_no_ascii_en_literales_de_pantalla(self):
        # los comentarios pueden llevar lo que quieran; los literales, no:
        # la fuente GFX es ASCII y un acento sale como basura
        for nombre, texto in self.ino.items():
            for lineno, line in enumerate(texto.splitlines(), 1):
                code = line.split('//')[0]
                for chunk in code.split('"')[1::2]:
                    self.assertTrue(chunk.isascii(),
                                    f'{nombre}:{lineno}: literal no ASCII: {chunk!r}')

    def test_las_llaves_estan_equilibradas(self):
        for nombre, texto in self.ino.items():
            self.assertEqual(texto.count('{'), texto.count('}'),
                             f'llaves descompensadas en {nombre}')



class TestChinoEnLaFuente(unittest.TestCase):
    """Todo el chino que se pinta esta en la fuente recortada (zhfont.h).

    Arduino_GFX no dibuja nada si falta el glifo (ni avanza el cursor), asi que
    un caracter que no este en la fuente desapareceria sin avisar. La fuente
    lleva solo lo que se usa: al cambiar un texto o un nombre hay que
    regenerarla con tools/make_zh_font.py, y este test avisa si se olvida.
    """

    def test_textos_y_nombres_chinos_estan_en_la_fuente(self):
        import make_zh_font

        with open(os.path.join(ROOT, 'tools', 'zh_font_chars.txt'), encoding='utf-8') as fh:
            en_fuente = set(fh.read().strip())
        usados = {chr(c) for c in make_zh_font.used_chars()}
        self.assertTrue(usados)
        faltan = ''.join(sorted(usados - en_fuente))
        self.assertFalse(faltan, f'faltan en la fuente (python3 tools/make_zh_font.py <bdf>): {faltan}')
        self.assertIn('椪', en_fuente)  # el de Ogerpon, que GB2312 no tenia

    def test_el_codificador_de_fuentes_lee_lo_que_escribe(self):
        import make_zh_font as m

        # glifos de mentira: un cuadrado, una cruz, uno vacio (espacio) y uno unicode
        cuadro = [[1] * 6 for _ in range(6)]
        cruz = [[1 if r == 3 or c == 3 else 0 for c in range(7)] for r in range(7)]
        glifos = {32: (0, 0, 0, 0, 4, []), 65: (6, 6, 1, 0, 8, cuadro), 103: (7, 7, 0, -2, 8, cruz),
                  0x4E2D: (7, 7, 0, -1, 9, cruz), 0x692A: (6, 6, 2, 1, 10, cuadro)}
        fuente = m.build_font(glifos)
        for c, g in glifos.items():
            self.assertEqual(m.decode_font(fuente, c), g, chr(c))
        self.assertIsNone(m.decode_font(fuente, 0x4E00))


class TestBuffersDeTexto(unittest.TestCase):
    """Cada snprintf(buf, sizeof(buf), T(S_x), ...) cabe en su buffer en TODOS los idiomas.

    Los buffers se dimensionaron cuando toda cadena era de un byte por
    caracter. En UTF-8 un kana o una silaba hangul ocupan 3, y snprintf corta
    en silencio: a veces se pierden cifras ("きろく 2" por "きろく 219") y a
    veces corta a mitad de un caracter, que la fuente ya no sabe dibujar. No lo
    avisa ni el compilador ni la pantalla, asi que se comprueba aqui con el
    peor caso de cada argumento segun su tipo:

      %u / %d  -> 5 cifras (todos los argumentos son de 16 bits o menos)
      %lu      -> 10 cifras
      %s       -> lo que declare ARGS_S; un %s sin declarar hace fallar el
                  test, para que quien lo anada piense cuanto puede ocupar
    """

    ARGS_S = {  # que puede llegar por cada %s, en orden
        'S_NAME_FMT': ('estrella', 'nombre_o_apodo'),
        'S_RELEASE_FMT': ('nombre',),
        'S_FAREWELL_BTN': ('nombre_o_apodo',),
        'S_RUNAWAY_BTN': ('nombre_o_apodo',),
        'S_REGION_DONE_FMT': ('region',),
        'S_VISIT_FMT': ('nombre_o_apodo',),
        'S_HOL_BIRTHDAY': ('dueño',),
        'S_SAY_LOVE': ('dueño',),
    }
    CIFRAS = {'u': 5, 'd': 5, 'lu': 10}
    ESPEC = re.compile(r'%0?\d*(lu|u|d|s)')

    @classmethod
    def setUpClass(cls):
        import test_i18n_formats as fmt
        from dex_data import DEX
        from dex_names import LOCAL_NAMES

        def leer(nombre):
            with open(os.path.join(ROOT, nombre), encoding='utf-8') as fh:
                return fh.read()

        cls.LANGS = fmt.LANGS
        cuerpo = re.search(r'enum\s+StrId[^{]*\{(.*?)\}', leer('i18n.h'), re.S).group(1)
        ids = re.findall(r'\b(S_[A-Z0-9_]+)\b', cuerpo)
        filas = fmt.extract_table(leer('i18n.cpp'), 'STRINGS', len(cls.LANGS))
        cls.LIT = {sid: [fila[k] for fila in filas] for k, sid in enumerate(ids)}
        cls.ino = {f: leer(f).splitlines() for f in sorted(os.listdir(ROOT)) if f.endswith('.ino')}
        cls.APODO = int(re.search(r'char\s+nick\[(\d+)\]', leer('pet.h')).group(1)) - 1
        cls.DUENO = int(re.search(r'sizeof\(OWNER_NAME\)\s*<=\s*(\d+)', leer('config.h')).group(1)) - 1
        base = max(len(row[2].encode()) for row in DEX)
        cls.NOMBRE = {}
        for lang in cls.LANGS:
            propios = [n[lang.lower()] for n in LOCAL_NAMES.values() if lang.lower() in n]
            cls.NOMBRE[lang] = max([base] + [len(n.encode('utf-8')) for n in propios])

    @staticmethod
    def bytes_literal(literal):
        """Bytes que ocupa en memoria un literal C escrito como en el fuente."""
        cuerpo = literal[1:-1].encode('utf-8')
        n = i = 0
        while i < len(cuerpo):
            if cuerpo[i] == 0x5C:  # barra invertida: \ooo octal, o \n \" \\ ...
                j = i + 1
                while j < len(cuerpo) and j < i + 4 and 0x30 <= cuerpo[j] <= 0x37:
                    j += 1
                i = j if j > i + 1 else i + 2
            else:
                i += 1
            n += 1
        return n

    def peor_caso(self, sid, li):
        lang, lit = self.LANGS[li], self.LIT[sid][li]
        total = self.bytes_literal(lit) + 1  # + terminador
        pendientes = list(self.ARGS_S.get(sid, ()))
        for m in self.ESPEC.finditer(lit):
            total -= len(m.group(0))
            if m.group(1) != 's':
                total += self.CIFRAS[m.group(1)]
                continue
            self.assertTrue(pendientes, f'{sid}: tiene un %s sin declarar en ARGS_S')
            arg = pendientes.pop(0)
            if arg == 'estrella':
                total += 1
            elif arg == 'nombre':
                total += self.NOMBRE[lang]
            elif arg == 'nombre_o_apodo':
                total += max(self.NOMBRE[lang], self.APODO)
            elif arg == 'dueño':  # OWNER_NAME (config.h, con su tope)
                total += self.DUENO
            elif arg == 'region':
                total += max(self.bytes_literal(self.LIT[f'S_REGION_{k}'][li]) for k in range(1, 10))
        return total

    def test_cada_texto_formateado_cabe_en_su_buffer_en_todos_los_idiomas(self):
        llamada = re.compile(r'snprintf\((\w+),\s*sizeof\(\1\),\s*T\((S_[A-Z0-9_]+)\)')
        revisadas = 0
        for fichero, lineas in self.ino.items():
            for n, linea in enumerate(lineas, 1):
                m = llamada.search(linea.split('//')[0])
                if not m:
                    continue
                var, sid = m.groups()
                tam = next((int(d.group(1)) for k in range(n - 2, -1, -1)
                            for d in [re.search(r'\bchar\s+(?:\w+\s*\[\d+\]\s*,\s*)*' + var + r'\s*\[(\d+)\]', lineas[k])]
                            if d), None)
                if tam is None:  # buffer global: esta en el principal, que va delante
                    tam = next((int(d.group(1)) for l in self.ino['TamaPoke.ino']
                                for d in [re.match(r'char\s+' + var + r'\s*\[(\d+)\]', l)] if d), None)
                self.assertIsNotNone(tam, f'{fichero}:{n}: no encuentro la declaracion de {var}')
                revisadas += 1
                for li, lang in enumerate(self.LANGS):
                    with self.subTest(fichero=fichero, linea=n, id=sid, idioma=lang):
                        peor = self.peor_caso(sid, li)
                        self.assertLessEqual(
                            peor, tam,
                            f'{fichero}:{n}: {var}[{tam}] con {sid} en {lang} necesita {peor} bytes')
        self.assertGreater(revisadas, 10, 'el patron de snprintf ya no encuentra las llamadas')

if __name__ == '__main__':
    unittest.main(verbosity=2)
