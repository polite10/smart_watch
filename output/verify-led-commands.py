"""Compile/run the pure command parser on the host with UB sanitization.

Pass --address to additionally enable AddressSanitizer on supported hosts.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--address', action='store_true')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='watch-led-commands-') as directory:
    binary = Path(directory) / 'test-led-commands'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=' + ('address,undefined' if args.address else 'undefined'),
                    '-fno-omit-frame-pointer', '-g',
                    '-I' + str(ROOT / 'App'), str(ROOT / 'output/test-led-commands.c'),
                    str(ROOT / 'App/services/watch_led_commands.c'), '-o', str(binary)], check=True, timeout=60)
    subprocess.run([str(binary)], check=True, timeout=30)
