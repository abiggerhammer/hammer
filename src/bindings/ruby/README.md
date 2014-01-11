# hammer-parser

Ruby bindings for [hammer](https://github.com/UpstandingHackers/hammer), a parsing library.


## Notes

* I called the gem `hammer-parser`, since there already is a [gem named `hammer`](https://rubygems.org/gems/hammer).


## Development

1. `cd src/bindings/ruby`.

2. Run `bundle install` to install dependencies.

3. Run `bundle console` to open `irb` with hammer loaded.

4. To run tests, just run `bundle exec rake test`.


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
  .choice(Hammer::Parser.token('Mom'), Hammer::Parser.token('Dad'))
  .token('!')
  .build
```

More like hammer in C:

```ruby
h = Hammer::Parser
parser = h.sequence(h.token('Hello '), h.choice(h.token('Mom'), h.token('Dad')), h.token('!'))
```

### Parsing

```ruby
result = parser.parse 'Hello Mom!'
=> #<HParseResult>
result = parser.parse 'Hello Someone!'
=> nil
```

The `parse` method returns an `HParseResult` object, which needs to be
kept around until you're entirely done with the parse tree, which can
be accessed with `result.ast`.

While the AST can be accessed using the same interface as the C
HParsedToken type, we recommend using `result.ast.unmarshal` instead.
This converts the entire parse tree into a standalone Ruby-native
datastructure which will likely be much easier to work with.
