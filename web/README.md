# TamaPoke web installer

A page that flashes this fork from the browser (Chrome or Edge on a computer),
with no Arduino and no drivers. It uses
[ESP Web Tools](https://esphome.github.io/esp-web-tools/). Live at
<https://mu-muu.github.io/TamaPoke/web/>.

## Contents

- `index.html`: the page (English and 简体中文). Step 1 flashes the firmware;
  step 2 links to `tamapoke-sd.zip` on the latest GitHub Release, which the
  user unzips onto the microSD; step 3 is the Wi-Fi setup.
- `manifest.json`: ESP Web Tools config. It flashes four parts at their own
  offsets (bootloader `0x0`, partition table `0x8000`, `boot_app0` `0xe000`,
  app `0x10000`) instead of one merged image, so the NVS at `0x9000` (pet,
  points, Wi-Fi) survives an update as long as "Erase device" stays unticked.
- `firmware/*.bin`: the four parts.

The public firmware is built **without** `config_private.h`, so it has the
neutral settings from `config.h`: New York time and weather, no chess.com
points.

## Regenerate

```bash
bash tools/build_web.sh
```

It compiles `HEAD` in a clean temporary worktree (no `config_private.h`, no
uncommitted changes), copies the four parts into `web/firmware/`, stamps the
version into `manifest.json`, and zips `tools/sdcard/mons` into
`build/tamapoke-sd.zip` (about 10 MB). Commit `web/`, push, and upload the zip
to a new GitHub Release as `tamapoke-sd.zip`; the page always links to the
latest release.

## Test locally

Web Serial needs a secure context (`https://` or `http://localhost`):

```bash
cd web && python3 -m http.server 8000
# open http://localhost:8000 in Chrome or Edge
```

## Deploy

GitHub Pages serves the `mods` branch from `/` (the empty `.nojekyll` at the
root skips the Jekyll build), so the page lives under `/web/`.

## Updating ESP Web Tools

`index.html` loads an exact version from unpkg with Subresource Integrity.
After changing the version, recompute the hash:

```bash
curl -s https://unpkg.com/esp-web-tools@X.Y.Z/dist/web/install-button.js \
  | openssl dgst -sha384 -binary | openssl base64 -A
```

The hash only covers that entry file, not the chunks it imports.
