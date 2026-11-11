#!/bin/bash
# Regenera el instalador web: web/firmware/*.bin, la version de web/manifest.json
# y build/tamapoke-sd.zip (para subirlo a una Release de GitHub).
#
# Compila HEAD en un worktree limpio, asi el firmware publico no lleva
# config_private.h (nombre, usuario de chess.com, ubicacion: sale con los
# valores neutros de config.h) ni cambios sin confirmar.
# Uso: bash tools/build_web.sh
set -e
cd "$(dirname "$0")/.."
ROOT=$PWD
FQBN="esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"
TMP=$(mktemp -d)
trap 'git -C "$ROOT" worktree remove --force "$TMP/TamaPoke" >/dev/null 2>&1; rm -rf "$TMP"' EXIT

git worktree add --detach "$TMP/TamaPoke" HEAD >/dev/null
REV=$(git rev-parse --short HEAD)
VER=$(sed -n 's/^#define FW_VERSION "\(.*\)"/\1/p' TamaPoke.ino)
echo "Compilando $REV (v$VER) sin ajustes personales..."
# rutas sin el nombre de usuario: __FILE__ de asserts y bibliotecas acaban en el firmware
MAP="-ffile-prefix-map=$HOME=/home/builder -ffile-prefix-map=/$HOME=/home/builder -ffile-prefix-map=$TMP=/src"
arduino-cli compile --fqbn "$FQBN" --build-path "$TMP/build" \
  --build-property "compiler.c.extra_flags=$MAP" --build-property "compiler.cpp.extra_flags=$MAP" \
  --build-property "compiler.S.extra_flags=$MAP" "$TMP/TamaPoke" | grep -E 'Sketch uses'
if LC_ALL=C grep -q -a -e "$HOME" -e "$TMP" "$TMP/build/TamaPoke.ino.bin"; then
  echo "ERROR: el firmware aun lleva rutas locales"; exit 1
fi

# Por partes y no el .merged.bin: este ocupa los 16 MB enteros y borraria la
# NVS (0x9000: partida, puntos, wi-fi) en cada actualizacion.
B="$TMP/build"
cp "$B/TamaPoke.ino.bootloader.bin" web/firmware/bootloader.bin   # 0x0
cp "$B/TamaPoke.ino.partitions.bin" web/firmware/partitions.bin   # 0x8000
cp "$B/boot_app0.bin"               web/firmware/boot_app0.bin    # 0xe000
cp "$B/TamaPoke.ino.bin"            web/firmware/tamapoke.bin     # 0x10000
sed -i '' "s/\"version\": \".*\"/\"version\": \"$VER-fork $REV\"/" web/manifest.json
echo "OK -> web/firmware/ ($(du -h web/firmware/tamapoke.bin | cut -f1)), manifest $VER-fork $REV"

echo "Empaquetando la SD..."
mkdir -p build
rm -f build/tamapoke-sd.zip
(cd tools/sdcard && zip -q -r -X "$ROOT/build/tamapoke-sd.zip" mons -x '*.DS_Store')
echo "OK -> build/tamapoke-sd.zip ($(du -h build/tamapoke-sd.zip | cut -f1)): subelo como tamapoke-sd.zip a la Release"
