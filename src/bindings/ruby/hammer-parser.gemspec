#encoding: UTF-8
Gem::Specification.new do |s|
  s.name          = 'hammer-parser'
  s.version       = '0.1.0'
  s.summary       = 'Ruby bindings to the hammer parsing library.'
  s.description   = s.summary # TODO: longer description?
  s.authors       = ['Meredith L. Patterson', 'TQ Hirsch', 'Jakob Rath']
  # TODO:
  # s.email = ...
  # s.homepage = ...

  files = []
  files << 'README.md'
  files << [
    "lib/hammer/internal.rb",
    "lib/hammer/parser.rb",
    "lib/hammer/parser_builder.rb",
    "lib/hammer.rb",
    "lib/minitest/hamer-parser_plugin.rb",
    "test/autogen_test.rb",
    "test/parser_test.rb"
  ]
  s.files = files
  s.test_files = s.files.select { |path| path =~ /^test\/.*_test.rb/ }

  s.require_paths = %w[lib]

  s.add_dependency 'ffi', '~> 1.9'
  s.add_dependency 'docile', '~> 1.1' # TODO: Find a way to make this optional
end

