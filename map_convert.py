#! /usr/bin/env python

import json
import os
import struct

def convert_map(filename, data):
    filename = filename.split('.')[0]
    bg = None
    heightmap = None
    for layer in data['layers']:
        if layer['name'] == 'bg':
            bg = layer
        elif layer['name'] == 'heightmap':
            heightmap = layer

    dest = open('build/data/levels/%s.ll' % filename, 'wb')
    dest.write(struct.pack('I', layer['width']))
    dest.write(struct.pack('I', layer['height']))
    for t in bg['data']:
        dest.write(struct.pack('B', t))
    for t in heightmap['data']:
        dest.write(struct.pack('B', t))
    dest.close()

if __name__ == '__main__':
    dirname = 'data/maps/'
    for fn in os.listdir('data/maps/'):
        fh = open(dirname + fn, 'r')
        convert_map(fn, json.loads(fh.read()))
        fh.close()
