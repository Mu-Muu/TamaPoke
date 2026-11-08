#!/usr/bin/env python3
"""Busca datos personales antes de publicar (y desde el hook pre-push).

Palabras prohibidas (sin distinguir mayusculas), todas locales y fuera del repo:
  - los valores de texto de config_private.h (nombre, usuario, coordenadas...)
  - cada linea de .privacy_denylist (una palabra o frase por linea; # = comentario)
  - git config user.name y user.email

  python3 tools/privacy_check.py               archivos que se subirian ahora (index + nuevos)
  python3 tools/privacy_check.py --tree REV    todos los archivos de un commit
  python3 tools/privacy_check.py --commits REV commits de REV que ningun remoto tiene:
                                               archivos, mensaje, autor y zona horaria (+0000)
  python3 tools/privacy_check.py --hook        lo que llama .git/hooks/pre-push (lee stdin)

Sale con 1 (y dice donde) si encuentra algo. No imprime contenido de archivos.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
ZERO = '0' * 40


def git(*args, data=None):
    return subprocess.run(['git', *args], cwd=ROOT, input=data, capture_output=True, check=True).stdout


def terms():
    found = set()
    priv = os.path.join(ROOT, 'config_private.h')
    if os.path.exists(priv):
        with open(priv, encoding='utf-8') as fh:
            for m in re.finditer(r'#define\s+\w+\s+"([^"]+)"', fh.read()):
                found.add(m.group(1))
    deny = os.path.join(ROOT, '.privacy_denylist')
    if os.path.exists(deny):
        with open(deny, encoding='utf-8') as fh:
            for line in fh:
                line = line.strip()
                if line and not line.startswith('#'):
                    found.add(line)
    for key in ('user.name', 'user.email'):
        try:
            v = git('config', key).decode().strip()
        except subprocess.CalledProcessError:
            v = ''
        if v:
            found.add(v)
    if not found:
        sys.exit('privacy_check: no hay palabras (falta config_private.h o .privacy_denylist)')
    return sorted({t.lower().encode('utf-8') for t in found if len(t) >= 3})


def hits_in(data, words):
    low = data.lower()
    return [w.decode('utf-8') for w in words if w in low]


def scan_blobs(entries, words, label):
    """entries: [(ruta, sha del blob, o None = leer el archivo del disco)]"""
    bad = []
    for path, sha in entries:
        if sha:
            data = git('cat-file', 'blob', sha)
        else:
            with open(os.path.join(ROOT, path), 'rb') as fh:
                data = fh.read()
        found = hits_in(data, words) + hits_in(path.encode('utf-8'), words)
        if found:
            bad.append(f'{label}: {path}: {", ".join(sorted(set(found)))}')
    return bad


def tree_entries(rev):
    out = git('ls-tree', '-r', '-z', rev).split(b'\0')
    entries = []
    for rec in out:
        if not rec:
            continue
        meta, path = rec.split(b'\t', 1)
        kind, sha = meta.split()[1:3]
        if kind == b'blob':
            entries.append((path.decode('utf-8'), sha.decode()))
    return entries


def worktree_entries():
    """Lo que se subiria ahora: archivos seguidos + nuevos no ignorados."""
    files = git('ls-files', '-z', '--cached', '--others', '--exclude-standard').split(b'\0')
    entries = []
    for f in files:
        if not f:
            continue
        path = f.decode('utf-8')
        if os.path.isfile(os.path.join(ROOT, path)):
            entries.append((path, None))
    return entries


def scan_commits(rev, words):
    bad = []
    shas = git('rev-list', rev, '--not', '--remotes').decode().split()
    for sha in shas:
        raw = git('cat-file', 'commit', sha)
        found = hits_in(raw, words)
        if found:
            bad.append(f'commit {sha[:9]}: mensaje/autor: {", ".join(sorted(set(found)))}')
        for line in raw.decode('utf-8', 'replace').splitlines():
            if not line:
                break
            m = re.match(r'(author|committer) .* ([+-]\d{4})$', line)
            if m and m.group(2) != '+0000':
                bad.append(f'commit {sha[:9]}: {m.group(1)} con zona horaria {m.group(2)} (debe ser +0000)')
        bad += scan_blobs(tree_entries(sha), words, f'commit {sha[:9]}')
    return bad


def report(bad, what):
    if bad:
        print(f'privacy_check: BLOQUEADO ({what})', file=sys.stderr)
        for b in bad:
            print('  ' + b, file=sys.stderr)
        return 1
    print(f'privacy_check: limpio ({what})')
    return 0


def main(argv):
    words = terms()
    if len(argv) >= 2 and argv[0] == '--tree':
        return report(scan_blobs(tree_entries(argv[1]), words, argv[1]), f'arbol {argv[1]}')
    if len(argv) >= 2 and argv[0] == '--commits':
        return report(scan_commits(argv[1], words), f'commits nuevos de {argv[1]}')
    if argv and argv[0] == '--hook':
        remote_url = argv[2] if len(argv) > 2 else ''
        if 'socquique' in remote_url.lower():
            print('privacy_check: no se sube al repositorio original', file=sys.stderr)
            return 1
        bad = []
        for line in sys.stdin.read().splitlines():
            parts = line.split()
            if len(parts) == 4 and parts[1] != ZERO:
                bad += scan_commits(parts[1], words)
        return report(bad, f'push a {remote_url}')
    return report(scan_blobs(worktree_entries(), words, 'local'), 'archivos que se subirian')


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
