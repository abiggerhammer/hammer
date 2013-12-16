% -*- prolog -*-

% test suite grammar
:- module(tsparser,
          [read_tc/1]).
:- expects_dialect(swi).
:- use_module(library(pure_input)).


nl --> "\r", !.
nl --> "\n", !.
ws --> " ", !.
ws --> "\t", !.
ws --> nl, !.
nonws --> ws, !, {fail}.
nonws --> [_]. % any character other than whitespace
tilEOL --> nl, !.
tilEOL --> [_], tilEOL.
comment --> "#", tilEOL.
layout1 --> ws, !; comment.
layout --> layout1, !, layout.
layout --> [].
eof([],[]).

idChar(Chr) -->
    [Chr],
    {memberchk(Chr, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")}.
idTailChar(Chr) --> idChar(Chr), !.
idTailChar(Chr) -->
    [Chr],
    {memberchk(Chr, "1234567890-_")}.

id(Name) --> % toplevel lexical token
    idChar(Chr),
    idTail(Rest),
    {atom_codes(Name, [Chr|Rest])}.
idTail([Chr|Rest]) -->
    idTailChar(Chr), !,
    idTail(Rest).
idTail([]) --> [].

string(Str) --> "\"", qstring(Str), "\"", !.
string(Str) --> "<", hexstring(Str), ">".

qchar(C) --> "\\x", !, hexbyte(C).
qchar(C) --> [C], {C >= 0x20, C < 0x7F}.
qstring([], [34|R], [34|R]) :- !.
qstring([C|Rest]) -->
    qchar(C), qstring(Rest).
qstring([]).
hexstring([C|Rest]) -->
    hexbyte(C), !,
    hexstring_post(Rest).
hexstring([]) -->
    [].
hexstring_post([C|Rest]) -->
    ".", !,
    hexbyte(C),
    hexstring_post(Rest).
hexstring_post([]) --> [].

hexchr(C) --> [C0], {memberchk(C0, "0123456789"), C is C0 - 0x30}, !.
hexchr(C) --> [C0], {memberchk(C0, "abcdef"), C is C0 - 0x61 + 10}, !.
hexchr(C) --> [C0], {memberchk(C0, "ABCDEF"), C is C0 - 0x41 + 10}, !.
hexbyte(C) -->
    hexchr(C0),
    hexchr(C1),
    {C is C0 * 16 + C1}.
hexnum(A) -->
    hexchr(C0),
    hexnum(A, C0).
hexnum(A, Acc) -->
    hexchr(C0), !,
    {AccP is Acc * 16 + C0},
    hexnum(A, AccP).
hexnum(Acc, Acc) --> [].


parse_result_seq([A|Ar]) -->
    parse_result(A), layout,
    parse_result_seq_tail(Ar).
parse_result_seq([]) -->
    layout.
parse_result_seq_tail([]) --> [].
parse_result_seq_tail([A|Ar]) -->
    ",", layout,
    parse_result(A), layout,
    parse_result_seq_tail(Ar).

parse_result(seq(A)) -->
    "[", layout,
    parse_result_seq(A), % eats trailing ws
    "]", !.
parse_result(char(A)) -->
    "'", qchar(A), "'", !.
parse_result(uint(A)) -->
    "u0x", hexnum(A), !.
parse_result(sint(A)) -->
    "s0x", hexnum(A), !.
parse_result(sint(A)) -->
    "s-0x", hexnum(A0), {A is -A0}, !.
parse_result(string(A)) -->
    string(A), !.
parse_result(none) --> "none".

parser_arg(A) --> parser(A), !.
parser_arg(string(A)) --> string(A), !.
parser_arg(char(A)) --> "'", qchar(A), "'", !.
parser_arg(num(A)) --> "0x", hexnum(A).

parser_arglist_tail([A0|As]) -->
    % eats leading whitespace
    ",", !, layout,
    parser_arg(A0), layout,
    parser_arglist_tail(As).
parser_arglist_tail([]) --> [].
    
parser_arglist([]) --> [].
parser_arglist([A0|As]) -->
    parser_arg(A0),
    layout,
    parser_arglist_tail(As). % eats trailing whitespace


parser(ref(Name)) -->
    "$", !, id(Name).
parser(parser(Name, Args)) -->
    id(Name), layout,
    "(", layout,
    parser_arglist(Args), % eats trailing whitespace
    ")".

testcase_elem(subparser(Name,Value)) -->
    "subparser", layout1, layout, !,
    "$", id(Name), layout,
    "=", layout,
    parser(Value), layout,
    ";".
testcase_elem(parser(Value)) -->
    "parser", layout1, layout, !,
    parser(Value), layout,
    ";".
testcase_elem(testFail(Input)) -->
    "test", layout1, layout,
    string(Input), layout,
    "-->", layout,
    "fail", layout,
    ";", !.
testcase_elem(test(Input, Expected)) -->
    "test", layout1, layout,
    string(Input), layout,
    "-->", layout,
    parse_result(Expected), layout,
    ";", !.
    
testcase_body([Elem|Rest]) -->
    testcase_elem(Elem), layout, !,
    testcase_body(Rest).
testcase_body([]) --> [].

testcase(testcase(Name,Content)) -->
    id(Name),
    layout,
    "{",
    layout,
    testcase_body(Content),
    layout,
    "}".
    
testcases([]) -->
    layout, eof, !.
testcases([TC|Rest]) -->
    layout,
    testcase(TC),
    testcases(Rest).
testsuite(A) -->
    testcases(A).
           

%slurp(File, Lst) :-
%    get_code(File, C),
%    (C == -1 ->
%        Lst = [];
%     slurp(File, Rest),
%     Lst = [C|Rest]), !.

read_tc(A) :-
    phrase_from_file(testsuite(A), 'test-suite').

%read_tc(A) :-
%    read_tc('test-suite', A).
%read_tc(Name, A) :-
%    open(Name, read, File),
%    slurp(File, Contents),
%    close(File), !,
%    phrase(testsuite(A), Contents).

