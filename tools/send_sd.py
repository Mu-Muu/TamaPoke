#!/usr/bin/env python3
"""Envia los sprites empaquetados a la SD de la placa por USB.

  python3 tools/send_sd.py                  # envia tools/sdcard/mons/*.bin
  python3 tools/send_sd.py --port /dev/cu.usbmodem101
  python3 tools/send_sd.py --ls             # lista lo que hay en la SD
  python3 tools/send_sd.py --port P tools/sdcard/mons/m*.bin   # solo esos
  python3 tools/send_sd.py --dir voice/1a2b3c4d voces/*.wav     # a otra carpeta (las voces: tools/voice.py)
"""
import argparse
import glob
import os
import sys
import time
import serial

def find_port():
    ports = glob.glob('/dev/cu.usbmodem*')
    if not ports:
        sys.exit("no encuentro la placa (/dev/cu.usbmodem*)")
    return ports[0]

def wait_line(ser, expect, timeout=10):
    end = time.time() + timeout
    while time.time() < end:
        line = ser.readline().decode(errors='replace').strip()
        if not line:
            continue
        print(f"  placa: {line}")
        if line == expect:
            return True
        if line == 'ERR':
            return False
    return False

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--port')
    ap.add_argument('--ls', action='store_true')
    ap.add_argument('--dir', default='mons', help='carpeta de la SD: mons (por defecto) o voice/<amigo>')
    ap.add_argument('files', nargs='*', help='solo estos .bin (por defecto, todo tools/sdcard/mons)')
    args = ap.parse_args()
    d = args.dir.strip('/')
    # antes de abrir el puerto: sin ficheros solo vale para mons (los sprites de siempre); con --dir
    # voice/<amigo> y sin *.wav se subian los 290 MB de sprites a esa carpeta
    if not args.ls and not args.files and d != 'mons':
        sys.exit(f"--dir {d}: di que ficheros subir (por ejemplo voces/*.wav)")

    port = args.port or find_port()
    print(f"puerto {port}")
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(1.5)
    ser.reset_input_buffer()

    if args.ls:
        ser.write(b'LS\n' if d == 'mons' else f'LS /{d}\n'.encode())
        wait_line(ser, 'DONE', 5)
        return

    files = sorted(args.files) or sorted(glob.glob(os.path.join(os.path.dirname(__file__), 'sdcard', 'mons', '*.bin')))
    if not files:
        sys.exit("no hay .bin; ejecuta antes tools/pack_pmd.py")

    fallos = 0
    for path in files:
        size = os.path.getsize(path)
        name = f"{d}/{os.path.basename(path)}"
        print(f"-> {name} ({size/1024:.0f} KB)")
        ser.write(f"PUT {name} {size}\n".encode())
        if not wait_line(ser, 'OK', 5):
            print("   la placa no acepto el PUT, sigo con el siguiente")
            fallos += 1
            continue
        t0 = time.time()
        ok = True
        with open(path, 'rb') as f:
            while chunk := f.read(2048):
                ser.write(chunk)
                # espera el ack '#' del bloque
                ack = ''
                while ack not in ('#', 'ERR'):
                    ack = ser.readline().decode(errors='replace').strip()
                    if ack == '':
                        ok = False
                        break
                if not ok or ack == 'ERR':
                    ok = False
                    break
        if ok and wait_line(ser, 'DONE', 30):
            kbs = size / 1024 / max(0.01, time.time() - t0)
            print(f"   ok ({kbs:.0f} KB/s)")
        else:
            print("   fallo la transferencia")
            fallos += 1
    if fallos:
        sys.exit(f"{fallos} de {len(files)} no se subieron")
    print("listo")

if __name__ == '__main__':
    main()
