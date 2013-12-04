# -*- cperl -*-
use warnings;
use strict;
use Test::More tests => 21;
use hammer;


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
