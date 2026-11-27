#!/usr/bin/env python3
"""Genera la lista de megaevoluciones de la guia desde mega.h."""
import json
import re
from pathlib import Path

from dex_names import LOCAL_NAMES


def main():
    root = Path(__file__).resolve().parent.parent
    header = (root / "mega.h").read_text(encoding="utf-8")
    table = re.search(r"\bMEGA_FORMS\s*\[\s*\]\s*=\s*\{(.*?)\};", header, re.S)
    if table is None:
        raise ValueError("No se encuentra MEGA_FORMS en mega.h")

    lines = [
        "// GENERADO por tools/gen_mega_list.py desde mega.h - no editar a mano.",
        "// Las megaevoluciones del juego: solo las que tienen sprites PMD en SpriteCollab.",
        "export type Mega = { dex: number; en: string; zh: string; tag: string };",
        "export const MEGAS: Mega[] = [",
    ]
    entry = r'\s*\{\s*(\d+)\s*,\s*"[^"]+"\s*,\s*"([^"]*)"\s*\}\s*,?\s*//(.*)'
    for line in table.group(1).splitlines():
        if not line.lstrip().startswith("{"):
            continue
        match = re.fullmatch(entry, line)
        if match is None:
            raise ValueError(f"Entrada de MEGA_FORMS no reconocida: {line}")
        number, tag, comment = match.groups()
        dex = int(number)
        en = json.dumps(comment.partition("(")[0].strip(), ensure_ascii=False)
        zh = json.dumps(LOCAL_NAMES[dex]["zh"], ensure_ascii=False)
        lines.append(f"  {{ dex: {dex}, en: {en}, zh: {zh}, tag: {json.dumps(tag)} }},")
    lines.append("];")

    output = root / "website" / "lib" / "megas.ts"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
