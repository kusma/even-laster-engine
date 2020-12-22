#!/bin/env python3
import os
import argparse

def main():
    parser = argparse.ArgumentParser(description='Create symlink')
    parser.add_argument('source', metavar='SOURCE')
    parser.add_argument('link', metavar='LINK')
    args = parser.parse_args()
    if os.path.islink(args.link):
        os.unlink(args.link)
    os.symlink(args.source, args.link, True)

if __name__ == "__main__":
    main()
