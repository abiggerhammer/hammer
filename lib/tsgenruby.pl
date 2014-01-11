% -*- prolog -*-
% Run with:
% $ swipl -q  -t halt -g tsgenruby:prolog tsgenruby.pl >output-file
% Note: this needs to be run from the lib/ directory.

% So, from the ruby directory
% (cd ../../../lib && swipl -q -t halt -g tsgenruby:prolog tsgenruby.pl ) >test/autogen_test.rb



:- module(tsgenruby,
          [gen_ts/2]).

:- expects_dialect(swi).
:- use_module(tsparser).
:- use_module(library(record)).

:- record testsuite_state(parser_no:integer = 0, test_no:integer=0).
% TODO: build a Box-like pretty-printer

to_title_case([], []) :- !.
to_title_case([WSep,S0|Ss], [R0|Rs]) :-
        memberchk(WSep, "_-"), !,
        code_type(R0, to_upper(S0)),
        to_title_case(Ss,Rs).
to_title_case([S0|Ss], [S0|Rs]) :-
        \+ memberchk(S0, "_-"),
        !, to_title_case(Ss,Rs).

format_parser_name(Name, Result) :-
    atom_codes(Name, CName),
    append("h.", CName, Result), !.

format_test_name(Name, Result) :-
    atom_codes(Name, CName),
    to_title_case([0x5f|CName], RName),
    append("Test", RName, Result), !.

indent(0) --> "", !.
indent(N) -->
    {N > 0},
    "  ",
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
        ({Args \= []} ->
        
         "(", pp_parser_args(Args), ")"
        ; "") .
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
        pp_parser(num(C)), ".chr". % Ruby is encoding-aware; this is a
                                   % more reasonable implementation

pp_parser(ref(Name)) -->
    {atom_codes(Name,CName)},
    "@sp_", CName, !.


pp_parser(A) -->
    { writef("WTF is a %w?\n", [A]),
      !, fail
    }.

upd_state_test_elem(parser(_), OldSt, NewSt) :- !,
        testsuite_state_parser_no(OldSt, OldRNo),
        NewRNo is OldRNo + 1,
        set_parser_no_of_testsuite_state(NewRNo, OldSt, NewSt).
upd_state_test_elem(test(_, _), OldSt, NewSt) :- !,
        testsuite_state_test_no(OldSt, OldTNo),
        NewTNo is OldTNo + 1,
        set_test_no_of_testsuite_state(NewTNo, OldSt, NewSt).
upd_state_test_elem(testFail(_), OldSt, NewSt) :- !,
        testsuite_state_test_no(OldSt, OldTNo),
        NewTNo is OldTNo + 1,
        set_test_no_of_testsuite_state(NewTNo, OldSt, NewSt).
upd_state_test_elem(_, St, St).

curparser_name(St) --> !,
        { testsuite_state_parser_no(St, RNo),
          format(string(X), "@parser_~w", RNo) },
        X.
curtest_name(St) --> !,
        { testsuite_state_test_no(St, RNo),
          format(string(X), "test_~w", RNo) },
        X.

pp_test_elem(decl, parser(_), _) --> !.
pp_test_elem(init, parser(P), St) -->
    !, indent(2),
    curparser_name(St), " = ",
    pp_parser(P),
    "\n".
pp_test_elem(exec, parser(_), _) --> !.
pp_test_elem(decl, subparser(Name,_), _) -->
    !, indent(2),
    pp_parser(ref(Name)),
    " = ",
    pp_parser(parser(indirect,[])),
    "\n".
pp_test_elem(init, subparser(Name, Parser), _) -->
    !, indent(2),
    pp_parser(ref(Name)), ".bind ",
    pp_parser(Parser),
    "\n".
pp_test_elem(exec, subparser(_,_), _) --> !.
pp_test_elem(decl, test(_,_), _) --> !.
pp_test_elem(init, test(_,_), _) --> !.
pp_test_elem(decl, testFail(_), _) --> !.
pp_test_elem(init, testFail(_), _) --> !.
pp_test_elem(exec, test(Str, Result), St) -->
    !,
    "\n",
    indent(1), "def ", curtest_name(St), "\n",
    indent(2), "assert_parse_ok ", curparser_name(St), ", ", pp_parser(string(Str)),
    ", ",
    pp_parse_result(Result),
    "\n",
    indent(1), "end\n".
pp_test_elem(exec, testFail(Str), St) -->
    !,
    "\n",
    indent(1), "def ", curtest_name(St), "\n",
    indent(2), "refute_parse_ok ", curparser_name(St), ", ", pp_parser(string(Str)), "\n",
    indent(1), "end\n".

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
    "[", pp_result_seq(Args), "]".
pp_parse_result(none) --> !,
    "nil".
pp_parse_result(uint(V)) --> !,
        pp_parser(num(V)).
pp_parse_result(sint(V)) --> !,
        pp_parser(num(V)).
pp_parse_result(string(A)) --> !,
        pp_parser(string(A)).

%pp_parse_result(A) -->
%    "\x1b[1;31m",
%    {with_output_to(codes(C), write(A))},
%    C,
%    "\x1b[0m".


pp_test_elems(Phase, Elems) -->
        { default_testsuite_state(State) },
        pp_test_elems(Phase, Elems, State).
pp_test_elems(_, [], _) --> !.
pp_test_elems(Phase, [X|Xs], St) -->
    !,
    { upd_state_test_elem(X, St, NewSt) },
    %{NewSt = St},
    pp_test_elem(Phase,X, NewSt),
    pp_test_elems(Phase,Xs, NewSt).

pp_test_case(testcase(Name, Elems)) -->
    !,
    { format_test_name(Name, TName) },
    indent(0), "class ", TName, " < Minitest::Test\n",
    indent(1), "def setup\n",
    indent(2), "super\n",
    indent(2), "h = Hammer::Parser\n",
    pp_test_elems(decl, Elems),
    pp_test_elems(init, Elems),
    indent(1), "end\n",
    pp_test_elems(exec, Elems),
    indent(0), "end\n\n".


pp_test_cases([]) --> !.
pp_test_cases([A|As]) -->
    pp_test_case(A),
    pp_test_cases(As).

pp_test_suite(Suite) -->
    "require 'bundler/setup'\n",
    "require 'minitest/autorun'\n",
    "require 'hammer'\n",
    pp_test_cases(Suite).

gen_ts(Foo,Str) :-
    phrase(pp_test_suite(Foo),Str).

prolog :-
    read_tc(A),
    gen_ts(A, Res),
    writef("%s", [Res]).
