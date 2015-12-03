#!/usr/bin/env python2

# Example parser: Base64, syntax only.
#
# Demonstrates how to construct a Hammer parser that recognizes valid Base64
# sequences.
#
# Note that no semantic evaluation of the sequence is performed, i.e. the
# byte sequence being represented is not returned, or determined. See
# base64_sem1.py and base64_sem2.py for examples how to attach appropriate
# semantic actions to the grammar.

from __future__ import print_function

import sys

import hammer as h


def init_parser():
    # CORE
    digit = h.ch_range(0x30, 0x39)
    alpha = h.choice(h.ch_range(0x41, 0x5a), h.ch_range(0x61, 0x7a))

    # AUX.
    plus = h.ch('+')
    slash = h.ch('/')
    equals = h.ch('=')

    bsfdig = h.choice(alpha, digit, plus, slash)
    bsfdig_4bit = h.in_('AEIMQUYcgkosw048')
    bsfdig_2bit = h.in_('AQgw')
    base64_3 = h.repeat_n(bsfdig, 4)
    base64_2 = h.sequence(bsfdig, bsfdig, bsfdig_4bit, equals)
    base64_1 = h.sequence(bsfdig, bsfdig_2bit, equals, equals)
    base64 = h.sequence(h.many(base64_3),
                        h.optional(h.choice(base64_2, base64_1)))

    return h.sequence(h.whitespace(base64), h.whitespace(h.end_p()))


def main():
    document = init_parser()

    s = sys.stdin.read()
    inputsize = len(s)
    print('inputsize=%i' % inputsize, file=sys.stderr)
    print('input=%s' % s, file=sys.stderr, end='')

    result = document.parse(s)

    if result:
        #print('parsed=%i bytes', result.bit_length/8, file=sys.stderr)
        print(result)


if __name__ == '__main__':
    import sys

    main()
