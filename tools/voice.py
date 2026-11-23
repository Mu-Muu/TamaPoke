#!/usr/bin/env python3
"""Convierte las grabaciones de un amigo (las del movil: m4a, mp3, wav...) en sus voces del correo.

  python3 tools/voice.py SALIDA grab00 grab01 ... grab13
  python3 tools/voice.py SALIDA --desde 7 grab07          # rehacer solo la 07

Las grabaciones van en el orden de voice.h: 00-11 las frases de la postal, en el orden del menu
(Hello!, Miss you!, Let's play!, See you tomorrow!, Good morning!, Sweet dreams!, Thank you!,
You can do it!, Happy birthday!, Had a great day!, Won at chess today!, My Pokemon evolved!),
12 una visita y 13 un regalo. Deja SALIDA/00.wav ... 13.wav en WAV PCM 16 kHz mono 16 bits, sin el
silencio del principio y del final, con el volumen igualado y como mucho 10 s. Despues se suben a la
SD de quien las va a oir:

  python3 tools/send_sd.py --dir voice/<amigo> SALIDA/*.wav

<amigo> = los 8 primeros hex del numero de jugador de quien grabo (MAIL INFO en la placa que las
recibe, en la lista de amigos). Necesita ffmpeg.
"""
import argparse
import os
import shutil
import subprocess
import sys

CLIPS = 14
# silencio fuera por los dos lados (dando la vuelta al audio) y volumen igualado entre frases
FILTRO = ('silenceremove=start_periods=1:start_threshold=-45dB:start_silence=0.05,areverse,'
          'silenceremove=start_periods=1:start_threshold=-45dB:start_silence=0.1,areverse,'
          'loudnorm=I=-16:TP=-2:LRA=11')


def convertir(src, dst):
    subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', src, '-af', FILTRO, '-ac', '1', '-ar', '16000',
                    '-sample_fmt', 's16', '-t', '10', '-map_metadata', '-1', '-fflags', '+bitexact',
                    '-flags:a', '+bitexact', dst], check=True)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('salida')
    ap.add_argument('grabaciones', nargs='+')
    ap.add_argument('--desde', type=int, default=0, help='numero de la primera (para rehacer solo algunas)')
    a = ap.parse_args(argv)
    if not shutil.which('ffmpeg'):
        sys.exit('falta ffmpeg (brew install ffmpeg)')
    if a.desde < 0 or a.desde + len(a.grabaciones) > CLIPS:
        sys.exit(f'como mucho {CLIPS} grabaciones, de la 00 a la {CLIPS - 1:02d}')
    os.makedirs(a.salida, exist_ok=True)
    for i, src in enumerate(a.grabaciones, a.desde):
        dst = os.path.join(a.salida, f'{i:02d}.wav')
        # primero a un temporal: si ffmpeg falla no queda un NN.wav roto (que send_sd.py subiria), y
        # convertir sobre el mismo fichero no lo trunca antes de leerlo
        tmp = os.path.join(a.salida, f'.{i:02d}.tmp.wav')
        try:
            convertir(src, tmp)
        except subprocess.CalledProcessError:
            if os.path.exists(tmp):
                os.remove(tmp)
            sys.exit(f'{i:02d}: ffmpeg no pudo con {src}')
        seg = (os.path.getsize(tmp) - 44) / 32000
        if seg < 0.2:  # casi todo era silencio (o mas bajo que -45 dB): no hay voz que guardar
            os.remove(tmp)
            sys.exit(f'{i:02d}: {os.path.basename(src)} no tiene voz (solo silencio); hay que grabarla otra vez')
        os.replace(tmp, dst)
        print(f'{i:02d}.wav  {seg:.1f} s  <- {os.path.basename(src)}')


if __name__ == '__main__':
    main()
