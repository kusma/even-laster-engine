#!/bin/env python
import os
import argparse
import xml.etree.ElementTree as ET
from struct import pack

def main():
    parser = argparse.ArgumentParser(description='Convert Rocket XML files into track-files.')
    parser.add_argument('files', metavar='FILE', nargs='*')
    args = parser.parse_args()
    destdir = os.environ['DESTDIR']
    for file in args.files:
        tree = ET.parse(file)
        root = tree.getroot()

        def track_path(base, name):
            def encode_byte(byte):
                char = chr(byte)
                if char == '.' or char == '_' or char == '/' or char.isalnum():
                    return char
                return "-{:02X}".format(byte)
            path = ''.join(map(encode_byte, bytearray(name.encode('utf-8'))))
            return "{}_{}.track".format(base, path)

        for track in root.iter('track'):
            path = os.path.join(destdir, track_path("data/sync", track.attrib['name']))
            file = open(path, 'wb')
            keys = track.getchildren()

            file.write(pack('<I', len(keys)))

            for key in keys:
                file.write(pack('<I', int(key.attrib['row'])))
                file.write(pack('<f', float(key.attrib['value'])))
                file.write(pack('<b', int(key.attrib['interpolation'])))
            file.close()

if __name__ == "__main__":
    main()
