# hammer-parser

Ruby bindings for [hammer](https://github.com/UpstandingHackers/hammer), a parsing library.


## Notes

* I called the gem `hammer-parser`, since there already is a [gem named `hammer`](https://rubygems.org/gems/hammer).

* C extension not really needed at the moment, if we don't mind hardcoding the token types in the ruby code.


## Development

1. `cd src/bindings/ruby`.

2. Run `bundle install` to install dependencies.

3. Run `rake compile` to compile the C extension.

4. Run `irb -I ./lib -r hammer` to open `irb` with hammer loaded.


## Installation

TODO



## Examples

### Building a parser

```ruby
parser = Hammer::Parser.build {
  token 'Hello '
  choice {
    token 'Mom'
    token 'Dad'
  }
  token '!'
}
```

Also possible:

```ruby
parser = Hammer::ParserBuilder.new
  .token('Hello ')
  .choice(Hammer::Parser::Token.new('Mom'), Hammer::Parser::Token.new('Dad'))
  .token('!')
  .build
```

More like hammer in C:

```ruby
h = Hammer::Parser
parser = h.sequence(h.token('Hello'), h.choice(h.token('Mom'), h.token('Dad')), h.token('!'))
```

### Parsing

```ruby
parser.parse 'Hello Mom!'
=> true
parser.parse 'Hello Someone!'
=> false
```

Currently you only get `true` or `false` depending on whether the parse succeeded or failed.
There's no way to access the parsed data yet.
