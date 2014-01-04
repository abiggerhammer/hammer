% -*- prolog -*-
% Run with:
% $ swipl -q  -t halt -g tsgencsharp:prolog tsgencsharp.pl >output-file
% Note: this needs to be run from the lib/ directory.

% So,
% swipl -q  -t halt -g tsgencsharp:prolog tsgencsharp.pl >../src/bindings/dotnet/test/hammer_tests.cs 


:- module(tsgencsharp,
          [gen_ts/2]).

:- expects_dialect(swi).
:- use_module(tsparser).

% TODO: build a Box-like pretty-printer

format_parser_name(Name, Result) :-
    atom_codes(Name, [CInit|CName]),
    code_type(RInit, to_upper(CInit)),
    append("Hammer.", [RInit|CName], Result), !.

format_test_name(Name, Result) :-
    atom_codes(Name, [CInit|CName]),
    code_type(RInit, to_upper(CInit)),
    append("Test", [RInit|CName], Result), !.

indent(0) --> "", !.
indent(N) -->
    {N > 0},
    "    ",
    {Np is N - 1},
    indent(Np).

pp_char_guts(0x22) -->
    "\\\"", !.
pp_char_guts(0x27) -->
    "\\'", !.
pp_char_guts(A) -->
    { A >= 0x20, A < 0x7F } ->
    [A];
    "\\x",
    { H is A >> 4, L is A /\ 0xF,
      code_type(Hc, xdigit(H)),
      code_type(Lc, xdigit(L)) },
    [Hc,Lc].

pp_hexnum_guts(0) --> !.
pp_hexnum_guts(A) -->
    { L is A /\ 0xF,
      H is A >> 4,
      code_type(Lc, xdigit(L)) },
    pp_hexnum_guts(H),
    [Lc], !.
pp_string_guts([]) --> !.
pp_string_guts([X|Xs]) -->
    pp_char_guts(X),
    pp_string_guts(Xs), !.

pp_parser_args([]) --> !.
pp_parser_args([X|Rest]) -->
    pp_parser(X),
    pp_parser_args_rest(Rest).
pp_parser_args_rest([]) --> !.
pp_parser_args_rest([X|Xs]) -->
    ", ",
    pp_parser(X),
    pp_parser_args_rest(Xs).

pp_parser(parser(Name, Args)) -->
    !,
    {format_parser_name(Name,Fname)},
    Fname,
    "(",
    pp_parser_args(Args),
    ")".
pp_parser(string(Str)) --> !,
    "\"",
    pp_string_guts(Str),
    "\"", !.
pp_parser(num(0)) --> "0", !.
pp_parser(num(Num)) --> !,
    ( {Num < 0} ->
      "-0x", {RNum is -Num}; "0x", {RNum = Num} ),
    pp_hexnum_guts(RNum).
pp_parser(char(C)) --> !,
    "'", pp_char_guts(C), "'", !.

pp_parser(ref(Name)) -->
    {atom_codes(Name,CName)},
    "sp_", CName, !.


pp_parser(A) -->
    { writef("WTF is a %w?\n", [A]),
      !, fail
    }.

pp_test_elem(decl, parser(_)) --> !.
pp_test_elem(init, parser(_)) --> !.
pp_test_elem(exec, parser(P)) -->
    !, indent(3),
    "parser = ",
    pp_parser(P),
    ";\n".
pp_test_elem(decl, subparser(Name,_)) -->
    !, indent(3),
    "IndirectParser ", pp_parser(ref(Name)),
    " = Hammer.Indirect();\n".
pp_test_elem(init, subparser(Name, Parser)) -->
    !, indent(3),
    pp_parser(ref(Name)), ".Bind(",
    pp_parser(Parser),
    ");\n".
pp_test_elem(exec, subparser(_,_)) --> !.
pp_test_elem(decl, test(_,_)) --> !.
pp_test_elem(init, test(_,_)) --> !.
pp_test_elem(decl, testFail(_)) --> !.
pp_test_elem(init, testFail(_)) --> !.
pp_test_elem(exec, test(Str, Result)) -->
    !, indent(3),
    "  CheckParseOK(parser, ", pp_parser(string(Str)),
    ", ",
    pp_parse_result(Result),
    ");\n".
pp_test_elem(exec, testFail(Str)) -->
    !, indent(3),
    "  CheckParseFail(parser, ", pp_parser(string(Str)),
    ");\n".

% pp_test_elem(_, _) --> !.

pp_result_seq([]) --> !.
pp_result_seq([X|Xs]) --> !,
    pp_parse_result(X),
    pp_result_seq_r(Xs).
pp_result_seq_r([]) --> !.
pp_result_seq_r([X|Xs]) --> !,
    ", ",
    pp_parse_result(X),
    pp_result_seq_r(Xs).

pp_byte_seq([]) --> !.
pp_byte_seq([X|Xs]) --> !,
    pp_parser(num(X)),
    pp_byte_seq_r(Xs).
pp_byte_seq_r([]) --> !.
pp_byte_seq_r([X|Xs]) --> !,
    ", ",
    pp_parser(num(X)),
    pp_byte_seq_r(Xs).

pp_parse_result(char(C)) --> !,
    %"(System.UInt64)",
    pp_parser(char(C)).
pp_parse_result(seq(Args)) --> !,
    "new object[]{ ", pp_result_seq(Args), "}".
pp_parse_result(none) --> !,
    "null".
pp_parse_result(uint(V)) --> !,
    "(System.UInt64)", pp_parser(num(V)).
pp_parse_result(sint(V)) --> !,
    "(System.Int64)(", pp_parser(num(V)), ")".
pp_parse_result(string(A)) --> !,
    "new byte[]{ ", pp_byte_seq(A), "}".
%pp_parse_result(A) -->
%    "\x1b[1;31m",
%    {with_output_to(codes(C), write(A))},
%    C,
%    "\x1b[0m".


pp_test_elems(_, []) --> !.
pp_test_elems(Phase, [X|Xs]) -->
    !,
    pp_test_elem(Phase,X),
    pp_test_elems(Phase,Xs).

pp_test_case(testcase(Name, Elems)) -->
    !,
    indent(2), "[Test]\n",
    { format_test_name(Name, TName) },
    indent(2), "public void ", TName, "() {\n",
    indent(3), "Parser parser;\n",
    pp_test_elems(decl, Elems),
    pp_test_elems(init, Elems),
    pp_test_elems(exec, Elems),
    indent(2), "}\n".


pp_test_cases([]) --> !.
pp_test_cases([A|As]) -->
    pp_test_case(A),
    pp_test_cases(As).

pp_test_suite(Suite) -->
    "namespace Hammer.Test {\n",
    indent(1), "using NUnit.Framework;\n",
    %indent(1), "using Hammer;\n",
    indent(1), "[TestFixture]\n",
    indent(1), "public partial class HammerTest {\n",
    pp_test_cases(Suite),
    indent(1), "}\n",
    "}\n".

gen_ts(Foo,Str) :-
    phrase(pp_test_suite(Foo),Str).

prolog :-
    read_tc(A),
    gen_ts(A, Res),
    writef("%s", [Res]).
