#!/usr/bin/python3

import pexpect
import os, sys, re, time
from   threading import Thread

class UnexpectedOutput(Exception):
    def __init__(self, expected, actual):
        super().__init__("Unexpected Output:\nHaving: <%s>\nExpected: <%s>" % (
            actual, expected))

class UnexpectedExitcode(Exception):
    def __init__(self, expected, actual, output):
        super().__init__("Unexpected Exitcode:\nHaving: %s\nExpected: %s\nOutput: %s" % (
            actual, expected, output))

class Test():
    __slots__ = ('binary', 'args', 'input', 'output', 'exitcode', 'confs', 'echo', 'sendeof')

    # "Run $binary with $arguments, send $input, expect $output and $exitcode
    #  (... and install $confs)"
    def __init__(self, binary, args, input=None, output=None, exitcode=None, confs=None, echo=True, sendeof=False):
        self.binary   = binary
        self.args     = args
        self.input    = input
        self.output   = output
        self.exitcode = exitcode
        self.confs    = confs
        self.echo     = echo
        self.sendeof  = sendeof

    def setup(self):
        if self.confs:
            for conf in self.confs:
                with open(conf[0], 'w') as fh:
                    fh.write(conf[1])

    def test(self):
        args = ['-C', 'mode global; bind Q signal kill;'] + list(self.args)
        pex  = pexpect.spawn(self.binary, args, echo=self.echo)

        if (self.input):
            pex.send(self.input)
            #for c in self.input:
            #    pex.send(c)
            #    time.sleep(1)
            if self.sendeof:
                pex.sendeof()

        time.sleep(1)

        if (self.exitcode is not None): # Process may have been killed
            pex.send('Q')

        time.sleep(1)

        try:
            rawout = pex.read_nonblocking(999, 1)
            output = rawout.decode('UTF-8')
            #print('OUTPUT=', output)
        except Exception as e:
            #print(e)
            rawout = ''
            output = ''

        if self.output:
            if type(self.output) is str:
                if self.output != output:
                    raise UnexpectedOutput(self.output, rawout)
            elif not self.output(output):
                raise UnexpectedOutput('(lambda)', rawout)

        pex.close()

        if (self.exitcode is not None and self.exitcode != pex.exitstatus):
            raise UnexpectedExitcode(self.exitcode, pex.exitstatus, output)

    def cleanup(self):
        if self.confs:
            for conf in self.confs:
                os.unlink(conf[0])

# =============================================================================
test_defs          = []
test_group         = ''
test_configuration = {}

def group(name, **defaults): # Set group name for tests
    global test_group
    test_group = name
    test_configuration = defaults

def set_defaults(**kwargs): # Set default test arguments 
    test_configuration = kwargs

def t(no, desc, *args, **kwargs): # Add test
    global test_defs
    kwargs.update(test_configuration)
    test_defs.append((("%s #%d" % (test_group, no)), desc, args, kwargs))

def BX(command, exec_command='cat', *args): # Bind key X to command, invoke `cat`
    return ['-C', 'bind X ' + command, exec_command] + list(args)

def _C(conf, exec_command='cat'): # Pass config using -C, invoke `cat`
    return ['-C', conf, exec_command]

def CL(*args): # Pass a command line, invoke `cat`
    return [*args] + ['cat']

def RE(pattern): # Match output against regex
    return lambda output: re.findall(pattern, output)

TEST_CONF_LOCATION = '/tmp/tkremap.test.conf'
TEST_CONF_CONTENT = '''
mode default
    bind A write D_A

mode global

mode test
    bind A write T_A
    bind b      write B
    bind b c    write C
    bind b c d  write D
'''
CFG = ['-c', TEST_CONF_LOCATION]

# === Basic tests =============================================================
group('Basic')
t(1,  'Call without arguments',   [],                       exitcode=201)
t(2,  'Call with true program',   ['true'],                 exitcode=0)
t(3,  'Call with false program',  ['false'],                exitcode=1)

# === Command line option tests ===============================================
group('Command line options')
t(1,  '-b (bind command)',        CL('-b', 'X', 'wr a'),    'X', 'a')
t(2,  '-k (remap key)',           CL('-k', 'X', 'a'),       'X', 'a')
t(3,  '-C (execute conf string)', CL('-C', 'bind X key a'), 'X', 'a')
t(4,  '-c (load conf file)',      CFG + CL(),               'A', 'D_A')
t(5,  '-m (switch mode)',         CFG + CL('-m', 'test'),   'A', 'T_A')
t(6,  '-u (unbind key)',          CFG + CL('-u', 'A'),      'A', '')
t(7,  '-v (start in vi mode)',    CL('-v'),                 'j', '^[OB')
t(8,  '-h (show help)',           CL('-h'),                 '',  RE('Usage:'))
t(9,  '-h commands',              CL('-h', 'commands'),     '',  RE('Commands'))
t(10, '-h all',                   CL('-h', 'all'),          '',  RE('Usage:'))
t(11, '-h keys',                  CL('-h', 'keys'),         '',  RE('Keys'))
t(12, '-h $command',              CL('-h', 'load'),         '',  RE('Load configuration'))

# === Behaviour ===============================================================
group('Default Behaviour')
t(1,   'Input==Output',           ['cat'],                  'X', 'X')
#t(1,  'Send ^C',                  ['cat'],                  chr(3), '', exitcode=130)
#t(2,  'Normal input')
#t(3,  '^Z)

# === Command tests ===========================================================
group('Command mask')
t(1,  'Simple',             BX('wr a; bind M mask'),        'XMX',  'aX')
t(2,  'State left?',        BX('wr a; bind M mask'),        'XMXX', 'aXa')

group('Command pass')
t(1,  'Simple',             BX('pass'),                     'X', 'X')

group('Command ignore')
t(1,  'Simple',             BX('ignore'),                   'X', '')

group('Command mode')
#t(1,  'Simple',             CF('mo test; bi Y wr a; mo default; bi X mo test'), 'XY', 'a')
# -p

group('Command exec')
t(1,  'STDOUT to STDOUT',   BX('ex printf a'),                 'X', 'a')
t(2,  'STDOUT to STDIN',    BX('ex -O- echo 1+1', 'bc'),       'X', '1+1\r\n2\r\n')
t(3,  'STDOUT to NULL',     BX('ex -O! echo test'),            'X', '')
t(4,  'STDERR to STDERR',   BX('ex cat NOSUCHFILE'),           'X', RE('NOSUCHFILE'))
t(5,  'STDERR to STDIN',    BX('ex -E- cat NOSUCHFILE', 'tr', 'A-Z', 'a-z'), 'X', RE('nosuchfile'))
t(6,  'STDERR to NULL',     BX('ex -E! cat NOSUCHFILE'),       'X', '')
t(7,  'Shell',              BX('ex -s "printf a; printf b"'),  'X', 'ab')
t(8,  'Exitcode true 1',    BX('{ ex true  && write a }'),     'X', 'a')
t(9,  'Exitcode true 2',    BX('{ ex true  || write a }'),     'X', '')
t(10, 'Exitcode false 1',   BX('{ ex false && write a }'),     'X', '')
t(11, 'Exitcode false 2',   BX('{ ex false || write a }'),     'X', 'a')
t(12, 'Ignore exitcode 1',  BX('{ ex -x false && write a }'),  'X', 'a')
t(13, 'Ignore exitcode 2',  BX('{ ex -x true  && write a }'),  'X', 'a')
t(14, 'Background',         BX('ex -b sleep 99; write a"'),    'X', 'a')
# TODO: option -I

group('Command bind')
t(1,  'With group',         _C('bind "{" { wr a; wr a }'),     '{', 'aa')
#t(2,  'Multi keys',         BX('Y write a'),                   'X', 'a') # TODO

group('Command unbind') # TODO

group('Command unbound') # TODO
t(1,  'char ignore (ignore char)',   _C('unbound char igno'),          'X', '')
t(1,  'char pass   (pass char)',     _C('unbound char pass'),          'X', 'X')
t(1,  'any ignore',                  _C('unbound any igno'),           'X', '')
t(1,  'any pass',                    _C('unbound any pass'),           'X', 'X')
t(1,  '[any] ignore',                _C('unbound igno'),               'X', '')
t(1,  '[any] pass',                  _C('unbound pass'),               'X', 'X')
#t(1,  'Unbound with group',         _C('unbound { wr a; wr a }'),  '{', 'aa')

group('Command readline', sendeof=True) # TODO
t(1,  'Simple',           BX('read'),                 'Xline\n',       'line\r\nline')
t(1,  'Write newline',    BX('read -n'),              'Xline\n',       'line\r\nline\r\nline\r\n')
t(1,  'Prompt',           BX('read -p PROMPT'),       'X',             'PROMPT')
t(1,  'Init text',        BX('read -i INIT'),         'X',             RE('INIT'))
t(1,  'Append text',      BX('read -A APPEND'),       'Xline\n',       RE('lineAPPEND'))
t(1,  'Prepend text',     BX('read -P PREPEND'),      'Xline\n',       RE('PREPENDline'))
t(1,  'Evaluate',         BX('read -!'),              'Xkey Ctrl-g\n', RE('\\^G'))
t(1,  'Confirm y',        BX('{ read -? && wr 1 }'),  'Xy',            '1')
t(1,  'Confirm Y',        BX('{ read -? && wr 1 }'),  'XY',            '1')
t(1,  'Confirm n',        BX('{ read -? || wr 0 }'),  'Xn',            '0')
t(1,  'Confirm N',        BX('{ read -? || wr 0 }'),  'XN',            '0')
t(1,  'Empty input',      BX('{ read || wr 0 }'),     'X\n',           '0')
t(1,  'Nonempty input',   BX('{ read && wr 1 }'),     'Xa\n',          RE('a1'))


# TODO: abbreviated command, ambigious


group('Command repeat')
t(1,  'Simple',             BX('wr a; repeat on'),          '4X',  'aaaa')
t(2,  'Zero',               BX('wr a; repeat on'),          '0X',  '') # TODO: actually should do something?
t(3,  'Two-Digits',         BX('wr a; repeat on'),          '10X', 'aaaaaaaaaa')
t(4,  'On, Off',            BX('wr a; rep on; rep off'),    '4X',  '4a')

group('Command rehandle') # TODO

group('Command key')
t(1,  'Simple',             BX('key a'),                    'X', 'a')
t(2,  'Concat',             BX('key a b'),                  'X', 'ab')
t(3,  'Repeat',             BX('key -r 4 a'),               'X', 'aaaa')
t(4,  'Repeat+Concat',      BX('key -r 2 a b'),             'X', 'abab')
t(5,  'Keysym',             BX('key PageUp'),               'X', '^[[5~')
t(6,  'Repeat=0',           BX('key -r 0 a'),               exitcode=22)
t(7,  'Repeat=-1',          BX('key -r -1 a'),              exitcode=22)
t(8,  'Invalid key',        BX('key INVALID'),              exitcode=205)

group('Command write')
t(1,  'Simple',             BX('write a'),                  'X', 'a')
t(2,  'Concat',             BX('write a b'),                'X', 'ab')
t(3,  'Repeat',             BX('write -r 4 a'),             'X', 'aaaa')
t(4,  'Repeat+Concat',      BX('write -r 2 a b'),           'X', 'abab')
t(5,  'Repeat=0',           BX('write -r 0 a'),             exitcode=22)
t(6,  'Repeat=-1',          BX('write -r -1 a'),            exitcode=22)

group('Command load') # TODO
# File Extension test! 

group('Command group')
t(1,  'Single',             BX('{ w a }'),                  'X', 'a')
t(2,  'Semicolon',          BX('{ w a ; w b }'),            'X', 'ab')
t(3,  'Semicolon 2',        BX('{ w a ; w b ; w c }'),      'X', 'abc')
t(4,  'And',                BX('{ w a && w b }'),           'X', 'ab')
t(5,  'And 2',              BX('{ w a && w b && w c }'),    'X', 'abc')
t(6,  'Or',                 BX('{ w a || w b }'),           'X', 'a')
t(7,  'Or 2',               BX('{ w a || w b || w c }'),    'X', 'a')
t(8,  'Mixed',              BX('{ w a && w b || w c }'),    'X', 'ab')
t(9,  'Nested',             BX('{ { w a ; w b } ; { w c ; w d } }'), 'X', 'abcd')

#group('Command signal')
#t(1,  'Parse INT',     BX('signal INT',    './strsignal'), 'X', 'SIGINT\r\n')
#t(2,  'Parse SIGINT',  BX('signal SIGINT', './strsignal'), 'X', 'SIGINT\r\n')
#t(3,  'Parse int',     BX('signal int',    './strsignal'), 'X', 'SIGINT\r\n')
#t(4,  'Parse sigint',  BX('signal sigint', './strsignal'), 'X', 'SIGINT\r\n')
#t(5,  'Parse 2',       BX('signal 2',      './strsignal'), 'X', 'SIGINT\r\n')
#t(6,  'Unknown Sig',   BX('signal XYZ'),                         exitcode=22)
#for sig in ('INT', 'HUP', 'TERM', 'USR1', 'USR2'):
#    t(99, 'Send SIG'+sig, BX('signal '+sig, './strsignal'), 'X', 'SIG'+sig+'\r\n')

# === Command option parsing tests =============================================
group('Option parsing')
t(1,  'Invalid option',       BX('write -I'),               exitcode=202)
t(2,  'Option terminator',    BX('write -- -r 4'),          'X', '-r4')
t(3,  'Option terminator',    BX('write -r 2 -- -r'),       'X', '-r-r')
t(4,  'Concat. opt + arg',    BX('write -r4 a'),            'X', 'aaaa')
#t('cmd opt #2', 'Concatenated', '-ir4)

# === Key parsing tests =======================================================
group('Key parsing')
key_tests = {
    'g':                'g',
    'G':                'G',
    'Shift-g':          'G',

    '^g':               '^G',
    '^G':               '^G',
    'Ctrl-g':           '^G',
    'Ctrl-G':           '^G',
    'Ctrl-Shift-g':     '^G',
    'Ctrl-Shift-G':     '^G',

    'Alt-g':            '^[g',
    'Alt-G':            '^[G',
    'Alt-Shift-g':      '^[G',
    'Alt-Shift-G':      '^[G',
    
    'Ctrl-Alt-g':       '^[^G',
    'Ctrl-Alt-G':       '^[^G',
    'Ctrl-Alt-Shift-g': '^[^G',
    'Ctrl-Alt-Shift-G': '^[^G',

    'F1':               '^[OP',
    'Shift-F1':         '^[[1;2P',
    'Alt-F1':           '^[[1;3P',
    'Alt-Shift-F1':     '^[[1;4P',
    'Ctrl-F1':          '^[[1;5P',
    'Ctrl-Shift-F1':    '^[[1;6P',

    'F12':              '^[[24~',
    'Shift-F12':        '^[[24;2~',
    'Alt-F12':          '^[[24;3~',
    'Alt-Shift-F12':    '^[[24;4~',
    'Ctrl-F12':         '^[[24;5~',
    'Ctrl-Shift-F12':   '^[[24;6~'
}
for key, expected in key_tests.items():
    t(0, key, BX('key '+key), 'X', expected)

    # Lower case
    key_ = re.sub('(Shift|Alt|Ctrl)', lambda m: m[0].lower(), key)
    t(0, key_, BX('key '+key_), 'X', expected)

    # Replace 'Alt' with 'Meta'
    key = key.replace('Alt', 'Meta')
    t(0, key, BX('key '+key), 'X', expected)

    # Shortened names: C-A-g
    key = key.replace('Shift', 'S')
    key = key.replace('Ctrl',  'C')
    key = key.replace('Meta',  'M')
    key = key.replace('Alt',   'A')
    t(0, key, BX('key '+key), 'X', expected)

# === Configuration parsing tests =============================================
group('Parsing')
t(1,  'Command<EOF>',         BX('write a'),                 'X', 'a')
t(2,  'Command;',             BX('write a;'),                'X', 'a')
t(3,  'Command\\n',           BX('write a\n'),               'X', 'a')
t(4,  'Command\\n\\n\\n',     BX('write a\n\n\n'),           'X', 'a')
t(5,  'Command;;;',           BX('write a ;;;'),             'X', exitcode=208)
t(6,  'Group spaced',         BX('{ wr a ; wr b }'),         'X', 'ab')
t(7,  'Group -1 space',       BX('{ wr a; wr b }'),          'X', 'ab')
t(8,  'Group -2 spaces',      BX('{wr a; wr b }'),           'X', 'ab')
t(9,  'Group -3 spaces',      BX('{wr a;wr b }'),            'X', 'ab')
t(10, 'Group no spaces',      BX('{wr a;wr b}'),             'X', 'ab')
t(11, 'Group with \\n',       BX('{wr a\nwr b}'),            'X', 'ab')
t(12, 'Group with \\n\n\n',   BX('{wr a\n\nwr b}'),          'X', 'ab')
t(13, '\\n\\n\\nCommand',     _C('\n\n\nbi X write a'),      'X', 'a')

# === Double quoted string parsing tests ======================================
group('Double Quoted')
dq_tests = {
    '\\a':      '^G',
    '\\b':      '^H',
    '\\e':      '^[',
    '\\f':      '^L',
    '\\n':      RE('\r\n'), # < Sometimes cat returns \r\n\r\n
    '\\r':      RE('\r\n'), # <-^
    '\\t':      '\t',
    '\\v':      '^K',
    '\\033':    '^[',
    '\\x1B':    '^[',
    '\\27':     '^[',
    '\\"':      '"'
}
for string, expected in dq_tests.items():
    t(0, string, BX('write "%s"'%string), 'X', expected)

# === Single quoted string parsing tests ======================================
group("Single Quoted")
t(1,  'Simple',               BX("write 'a'"),               'X', 'a')
t(2,  'No newline',           BX("write '\\n'"),             'X', 'n')
t(3,  'Escaped quote',        BX("write '\\\''"),            'X', '\'')

# === Comment parsing tests ===================================================
group("Comment parsing")
t(1,  'Simple',               BX('write a # comment'),       'X', 'a')
t(2,  'Double quoted',        BX('write "#"'),               'X', '#')
t(3,  'Single quoted',        BX("write '#'"),               'X', '#')

# === Words ===================================================================
group("Special chars")
t(1,  '& != &&',              BX('write &'),                 'X', '&')
t(2,  '| != ||',              BX('write |'),                 'X', '|')

# === Misc ====================================================================
group("Misc")

# === MAIN ====================================================================
TKREMAP_BIN = sys.argv[1]
THREADED    = True
IDs         = []

for ID in sys.argv[2:]:
    try:
        IDFrom, IDTo = ID.split('-')
        IDs.extend(range(int(IDFrom), int(IDTo) + 1))
    except:
        IDs.append(int(ID))

def run_test(ID, test_def, verbose=False):
    if verbose:
        print(test_def[0], '...')

    t = Test(TKREMAP_BIN, *test_def[2], **test_def[3])

    try:
        t.setup()
        t.test()
    except Exception as e:
        print("ID: %d\nName: %s\nDesc: %s\n%s\n" % (ID, test_def[0], test_def[1], e))
        raise SystemExit()
    finally:
        t.cleanup()

    return True

# Install global test configuration
with open(TEST_CONF_LOCATION, 'w') as fh:
    fh.write(TEST_CONF_CONTENT)

threads = []
for ID, test_def in enumerate(test_defs):
    if IDs and ID not in IDs:
        continue

    if THREADED:
        thr = Thread(target=run_test, args=(ID, test_def,))
        thr.start()
        threads.append(thr)
    else:
        if not run_test(ID, test_def, True):
            break

# Wait for threads
for thr in threads:
    thr.join()

print("Executed %d tests" % len(threads))

# Uninstall global test configuration
os.unlink(TEST_CONF_LOCATION)
