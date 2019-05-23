#!/usr/bin/python3

l = [
    'EOF',
    'LEX_TOKEN_WORD',
    'LEX_TOKEN_SEMICOLON',
    'LEX_TOKEN_AND',
    'LEX_TOKEN_OR',
    'LEX_TOKEN_BLOCK_BEG',
    'LEX_TOKEN_BLOCK_END',
    'LEX_TOKEN_NEW_LINE'
]

if_then = dict([ ('TOK == %s' % i,'"%s"' % i) for i in l ])

def print_ternary(if_then_values, else_value, end='\n', indent=' '):
    count = 0;
    for if_value, then_value in if_then_values.items():
        tabs = indent * count
        print("%s( %s ? %s : " % (tabs, if_value, then_value), end=end)
        count += 1

    tabs = indent * count
    print(tabs, else_value, end=' ')

    for i in range(count):
        print(end=")")
    print()

print_ternary(if_then, '"UNKNOWN"', '\\\n', '')

