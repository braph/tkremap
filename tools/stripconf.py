#!/usr/bin/python3

import re, sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('file')
parser.add_argument('-c', action='store_true', help='export as C string')
parser.add_argument('-v', default='conf',      help='variable name')
args = parser.parse_args()

words = {
    'bind':          'b',
   #'command':       'co',
    'exec':          'ex',
    'ignore':        'ig',
    'key':           'ke',
    'load':          'l',
    'mode':          'mo',
    'mask':          'ma',
    'repeat':        'rep',
    'rehandle':      'reh',
    'redraw_method': 'red',
    'readline':      'rea',
    'signal':        'si',
    'suspend':       'su',
    'unbind':        'unbi',
    'unbound':       'unbo',
    'write':         'wr',
    'pass':          'pa',
    'on':            'on',
    'off':           'of'
}

def strip_conf(s):
    r = ''
    for l in s.split('\n'):
        l = l.strip()
        l = l.rstrip(';')

        if not l:
            continue

        # comments
        if l.startswith('#'):
            continue
        l = re.sub('#.*', '', l)

        # strip space (general)
        l = re.sub(' +', ' ', l)

        # strip space from (operators)
        l = re.sub(' *&& *', '&&', l)
        l = re.sub(' *\\|\\| *', '||', l)
        l = re.sub(' *; *', ';', l)

        # compress arguments
        l = re.sub(' (-[a-zA-Z]) ([a-zA-Z0-9]+)', ' \\1\\2', l)

        for search, replace in words.items():
            l = l.replace(search, replace)

        r += '%s;\n' % l
    return r

def cstring(s):
    r = ''
    for l in s.split('\n'):
        if l:
            l = l.replace('\\', '\\\\')
            l = l.replace('"', '\\"')
            r += '"%s"\n' % l
    return r

with open(args.file, 'r') as fh:
    content = fh.read(-1)
    stripped = strip_conf(content)

    if args.c:
        stripped = cstring(stripped)
        stripped = 'const char *%s = \n%s;' % (args.v, stripped)

    print(stripped)
