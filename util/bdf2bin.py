#!/usr/bin/env python3

from bdfparser import Font
import argparse
import pathlib
import struct

def convert(infile):
    font = Font(infile)
    ret = b''
    bbx = font.headers['fbbx']
    bby = font.headers['fbby']

    for i in range(ord(' '), 127):
        glyph = font.glyph(chr(i))
        ret += bytes([0xff ^ b for b in glyph.draw().tobytes('1')])

    header = struct.pack('BBxx', font.headers['fbbx'], font.headers['fbby'])
    return header + ret

def main():
    parser = argparse.ArgumentParser(description='Convert BDF fonts to bin fmt')
    parser.add_argument('input', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path)
    args = parser.parse_args()

    with args.input.open('r') as infile, args.output.open('wb') as outfile:
        outfile.write(convert(infile))

if __name__ == '__main__':
    main()
