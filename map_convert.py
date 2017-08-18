#! /usr/bin/env python

import json
import os
import struct

def convert_map(filename, data):
    filename = filename.split('.')[0]
    layer = data['layers'][0]
    dest = open('build/data/levels/%s.ll' % filename, 'wb')
    dest.write(struct.pack('I', layer['width']))
    dest.write(struct.pack('I', layer['height']))
    for t in layer['data']:
        dest.write(struct.pack('B', t))
    dest.close()

if __name__ == '__main__':
    dirname = 'data/maps/'
    for fn in os.listdir('data/maps/'):
        fh = open(dirname + fn, 'r')
        convert_map(fn, json.loads(fh.read()))
        fh.close()
