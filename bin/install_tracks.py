#!/bin/env python
import shutil
import argparse
from glob import glob
import os

def main():
    parser = argparse.ArgumentParser(description='Install Rocket track-files.')
    parser.add_argument('dir', metavar='DIR')
    args = parser.parse_args()
    destdir = os.path.join(os.getenv('DESTDIR'), os.environ['MESON_INSTALL_PREFIX'], 'data')
    tracks = glob(os.path.join(args.dir, '*.track'))
    for track in tracks:
        shutil.copy2(track, destdir)

if __name__ == "__main__":
    main()
