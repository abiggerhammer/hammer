# -*- cperl -*-
use warnings;
use strict;
use Data::Dumper;
use Test::More tests => 41;
use hammer;

# differences from C version:

# - in takes any number of arguments, which are concatenated. This
#   makes ch_range irrelevant.
#
# - foo


sub check_parse_eq {
  my ($parser, $input, $expected) = @_;
  my $actual;
  eval {
    $actual = $parser->parse($input);
  };
  if ($@) {
    diag($@);
    ok($@ eq "");
  } else {
    #diag(Dumper($actual));
    is_deeply($actual, $expected);
  }
}

sub check_parse_failed {
  my ($parser, $input) = @_;
  eval {
    my $actual = $parser->parse($input);
  };
  ok($@ ne "");
}

subtest "token" => sub {
  my $parser = hammer::token("95\xa2");

  check_parse_eq($parser, "95\xa2", "95\xa2");
  check_parse_failed($parser, "95");
};

subtest "ch" => sub {
  my $parser = hammer::ch("\xa2");
  #check_parse_eq($parser, "\xa2", 0xa2);
  check_parse_eq($parser, "\xa2", "\xa2");
  check_parse_failed($parser, "\xa3");
};

subtest "ch_range" => sub {
  # ch_range doesn't need to be part of hammer-perl; the equivalent
  # effect can be achieved with hammer::in('a'..'z')
  #
  # However, the function is provided just in case.
  my $parser = hammer::ch_range('a','c');
  check_parse_eq($parser, 'b', 'b');
  #check_parse_eq($parser, 'b', 0x62);
  check_parse_failed($parser, 'd');
};

SKIP: {
  use integer;
  no warnings 'portable'; # I know the hex constants are not portable. that's why this test is skipped on <64 bit systems.
  skip "Needs 64-bit support", 2 if 0x4000000 * 2 eq -1; # TODO: Not sure if this works; may need $Config{ivsize} >= 8
  subtest "int64" => sub {
    my $parser = hammer::int64();
    check_parse_eq($parser, "\xff\xff\xff\xfe\x00\x00\x00\x00", -0x200000000);
    check_parse_failed($parser, "\xff\xff\xff\xfe\x00\x00\x00");
  };
  subtest "uint64" => sub {
    my $parser = hammer::uint64();
    check_parse_eq($parser, "\x00\x00\x00\x02\x00\x00\x00\x00", 0x200000000);
    check_parse_failed($parser, "\x00\x00\x00\x02\x00\x00\x00");
  };
}

subtest "int32" => sub {
  my $parser = hammer::int32();
  check_parse_eq($parser, "\xff\xfe\x00\x00", -0x20000);
  check_parse_eq($parser, "\x00\x02\x00\x00",  0x20000);
  check_parse_failed($parser, "\xff\xfe\x00");
  check_parse_failed($parser, "\x00\x02\x00");
};

subtest "uint32" => sub {
  my $parser = hammer::uint32();
  check_parse_eq($parser, "\x00\x02\x00\x00", 0x20000);
  check_parse_failed($parser, "\x00\x02\x00")
};

subtest "int16" => sub {
  my $parser = hammer::int16();
  check_parse_eq($parser, "\xfe\x00", -0x200);
  check_parse_eq($parser, "\x02\x00", 0x200);
  check_parse_failed($parser, "\xfe");
  check_parse_failed($parser, "\x02");
};

subtest "uint16" => sub {
  my $parser = hammer::uint16();
  check_parse_eq($parser, "\x02\x00", 0x200);
  check_parse_failed($parser, "\x02");
};

subtest "int8" => sub {
  my $parser = hammer::int8();
  check_parse_eq($parser, "\x88", -0x78);
  check_parse_failed($parser, "");
};

subtest "uint8" => sub {
  my $parser = hammer::uint8();
  check_parse_eq($parser, "\x78", 0x78);
  check_parse_failed($parser, "");
};

subtest "int_range" => sub { # test 12
  my $parser = hammer::int_range(hammer::uint8(), 3, 10);
  check_parse_eq($parser, "\x05", 5);
  check_parse_failed($parser, "\x0b");
};

subtest "whitespace" => sub {
  my $parser = hammer::whitespace(hammer::ch('a'));
  check_parse_eq($parser, "a", "a");
  check_parse_eq($parser, " a", "a");
  check_parse_eq($parser, "  a", "a");
  check_parse_eq($parser, "\t\n\ra", "a");
};

subtest "whitespace-end" => sub {
  my $parser = hammer::whitespace(hammer::end_p());
  check_parse_eq($parser, "", undef);
  check_parse_eq($parser, "  ", undef);
  check_parse_failed($parser, "  x", undef)
};

subtest "left" => sub { # test 15
  my $parser = hammer::left(hammer::ch('a'),
			    hammer::ch(' '));
  check_parse_eq($parser, "a ", "a");
  check_parse_failed($parser, "a");
  check_parse_failed($parser, " ");
};

subtest "right" => sub {
  my $parser = hammer::right(hammer::ch(' '),
			     hammer::ch('a'));
  check_parse_eq($parser, " a", "a");
  check_parse_failed($parser, "a");
  check_parse_failed($parser, " ");
};

subtest "middle" => sub {
  my $parser = hammer::middle(hammer::ch(' '),
			      hammer::ch('a'),
			      hammer::ch(' '));
  check_parse_eq($parser, " a ", "a");
  for my $test_string (split('/', "a/ / a/a / b /ba / ab")) {
    check_parse_failed($parser, $test_string);
  }
};

subtest "action" => sub {
  my $parser = hammer::action(hammer::sequence(hammer::choice(hammer::ch('a'),
							      hammer::ch('A')),
					       hammer::choice(hammer::ch('b'),
							      hammer::ch('B'))),
			      sub { [map(uc, @{+shift})]; });
  check_parse_eq($parser, "ab", ['A', 'B']);
  check_parse_eq($parser, "AB", ['A', 'B']);
  check_parse_eq($parser, 'Ab', ['A', 'B']);
  check_parse_failed($parser, "XX");
};


subtest "in" => sub {
  my $parser = hammer::in('a'..'c');
  check_parse_eq($parser, 'a', 'a');
  check_parse_eq($parser, 'b', 'b');
  check_parse_eq($parser, 'c', 'c');
  check_parse_failed($parser, 'd');
};

subtest "not_in" => sub { # test 20
  my $parser = hammer::not_in('a'..'c');
  check_parse_failed($parser, 'a');
  check_parse_failed($parser, 'b');
  check_parse_failed($parser, 'c');
  check_parse_eq($parser, 'd', 'd');
};

subtest "end_p" => sub {
  my $parser = hammer::sequence(hammer::ch('a'), hammer::end_p());
  check_parse_eq($parser, 'a', ['a']);
  check_parse_failed($parser, 'aa');
};

subtest "nothing_p" => sub {
  my $parser = hammer::nothing_p();
  check_parse_failed($parser, "");
  check_parse_failed($parser, "foo");
};

subtest "sequence" => sub {
  my $parser = hammer::sequence(hammer::ch('a'), hammer::ch('b'));
  check_parse_eq($parser, "ab", ['a','b']);
  check_parse_failed($parser, 'a');
  check_parse_failed($parser, 'b');
};

subtest "sequence-whitespace" => sub {
  my $parser = hammer::sequence(hammer::ch('a'),
				hammer::whitespace(hammer::ch('b')));
  check_parse_eq($parser, "ab", ['a', 'b']);
  check_parse_eq($parser, "a b", ['a', 'b']);
  check_parse_eq($parser, "a  b", ['a', 'b']);
  check_parse_failed($parser, "a  c");
};

subtest "choice" => sub { # test 25
  my $parser = hammer::choice(hammer::ch('a'),
			      hammer::ch('b'));
  check_parse_eq($parser, 'a', 'a');
  check_parse_eq($parser, 'b', 'b');
  check_parse_failed($parser, 'c');
};

subtest "butnot" => sub {
  my $parser = hammer::butnot(hammer::ch('a'), hammer::token('ab'));
  check_parse_eq($parser, 'a', 'a');
  check_parse_eq($parser, 'aa', 'a');
  check_parse_failed($parser, 'ab');
};

subtest "butnot-range" => sub {
  my $parser = hammer::butnot(hammer::ch_range('0', '9'), hammer::ch('6'));
  check_parse_eq($parser, '4', '4');
  check_parse_failed($parser, '6');
};

subtest "difference" => sub {
  my $parser = hammer::difference(hammer::token('ab'),
				  hammer::ch('a'));
  check_parse_eq($parser, 'ab', 'ab');
  check_parse_failed($parser, 'a');
};

subtest "xor" => sub {
  my $parser = hammer::xor(hammer::in('0'..'6'),
			   hammer::in('5'..'9'));
  check_parse_eq($parser, '0', '0');
  check_parse_eq($parser, '9', '9');
  check_parse_failed($parser, '5');
  check_parse_failed($parser, 'a');
};

subtest "many" => sub { # test 30
  my $parser = hammer::many(hammer::in('ab'));
  check_parse_eq($parser, '', []);
  check_parse_eq($parser, 'a', ['a']);
  check_parse_eq($parser, 'b', ['b']);
  check_parse_eq($parser, 'aabbaba', [qw/a a b b a b a/]);
};

subtest "many1" => sub {
  my $parser = hammer::many1(hammer::in('ab'));
  check_parse_eq($parser, 'a', ['a']);
  check_parse_eq($parser, 'b', ['b']);
  check_parse_eq($parser, 'aabbaba', [qw/a a b b a b a/]);
  check_parse_failed($parser, '');
  check_parse_failed($parser, 'daabbabadef');
};
subtest "repeat_n" => sub {
  my $parser = hammer::repeat_n(hammer::in('ab'), 2);
  check_parse_eq($parser, 'abdef', ['a','b']);
  check_parse_failed($parser, 'adef');
};

subtest "optional" => sub {
  my $parser = hammer::sequence(hammer::ch('a'),
				hammer::optional(hammer::in('bc')),
				hammer::ch('d'));
  check_parse_eq($parser, 'abd', [qw/a b d/]);
  check_parse_eq($parser, 'abd', [qw/a b d/]);
  check_parse_eq($parser, 'ad', ['a',undef,'d']);
  check_parse_failed($parser, 'aed');
  check_parse_failed($parser, 'ab');
  check_parse_failed($parser, 'ac');
};

subtest "ignore" => sub {
  my $parser = hammer::sequence(hammer::ch('a'),
				hammer::ignore(hammer::ch('b')),
				hammer::ch('c'));
  check_parse_eq($parser, "abc", ['a','c']);
  check_parse_failed($parser, 'ac');
};

subtest "sepBy" => sub { # Test 35
  my $parser = hammer::sepBy(hammer::in('1'..'3'),
			     hammer::ch(','));
  check_parse_eq($parser, '1,2,3', ['1','2','3']);
  check_parse_eq($parser, '1,3,2', ['1','3','2']);
  check_parse_eq($parser, '1,3', ['1','3']);
  check_parse_eq($parser, '3', ['3']);
  check_parse_eq($parser, '', []);
};

subtest "sepBy1" => sub {
  my $parser = hammer::sepBy1(hammer::in("123"),
			      hammer::ch(','));
  check_parse_eq($parser, '1,2,3', ['1','2','3']);
  check_parse_eq($parser, '1,3,2', ['1','3','2']);
  check_parse_eq($parser, '1,3', ['1','3']);
  check_parse_eq($parser, '3', ['3']);
  check_parse_failed($parser, '');
};

subtest "epsilon" => sub {
  check_parse_eq(hammer::sequence(hammer::ch('a'),
				  hammer::epsilon_p(),
				  hammer::ch('b')),
		 'ab', ['a','b']);
  check_parse_eq(hammer::sequence(hammer::epsilon_p(),
				  hammer::ch('a')),
		 'a', ['a']);
  check_parse_eq(hammer::sequence(hammer::ch('a'),
				  hammer::epsilon_p()),
		 'a', ['a']);
};


subtest "attr_bool" => sub {
  my $parser = hammer::attr_bool(hammer::many1(hammer::in('ab')),
				 sub { my ($a, $b) = @{+shift}; $a eq $b });
  check_parse_eq($parser, "aa", ['a','a']);
  check_parse_eq($parser, "bb", ['b','b']);
  check_parse_failed($parser, "ab");
};

subtest "and" => sub {
  check_parse_eq(hammer::sequence(hammer::and(hammer::ch('0')),
				  hammer::ch('0')),
		 '0', ['0']);
  check_parse_failed(hammer::sequence(hammer::and(hammer::ch('0')),
				      hammer::ch('1')),
		     '0');
  my $parser = hammer::sequence(hammer::ch('1'),
				hammer::and(hammer::ch('2')));
  check_parse_eq($parser, '12', ['1']);
  check_parse_failed($parser, '1');
  check_parse_failed($parser, '13');
};

subtest "not" => sub { # test 40
  # This is not how you'd *actually* write the parser for this
  # language; in case of Packrat, it's better to swap the order of the
  # arguments, and for other backends, the problem doesn't appear at
  # all.
  my $parser = hammer::sequence(hammer::ch('a'),
				hammer::choice(hammer::ch('+'),
					       hammer::token('++')),
				hammer::ch('b'));
  check_parse_eq($parser, 'a+b', ['a','+','b']);
  check_parse_failed($parser, 'a++b'); # ordered choice

  $parser = hammer::sequence(hammer::ch('a'),
			     hammer::choice(hammer::sequence(hammer::ch('+'),
							     hammer::not(hammer::ch('+'))),
					    hammer::token('++')),
			     hammer::ch('b'));
  check_parse_eq($parser, 'a+b', ['a',['+'],'b']);
  check_parse_eq($parser, 'a++b', ['a', '++', 'b']);
};

subtest "rightrec" => sub {
  my $parser = hammer::indirect();
  hammer::bind_indirect($parser,
			hammer::choice(hammer::sequence(hammer::ch('a'),
							$parser),
				       hammer::epsilon_p));
  check_parse_eq($parser, 'a', ['a']);
  check_parse_eq($parser, 'aa', ['a', ['a']]);
  check_parse_eq($parser, 'aaa', ['a', ['a', ['a']]]);
};
  
