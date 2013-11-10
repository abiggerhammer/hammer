require 'mkmf'

extension_name = 'hammer_ext'
dir_config extension_name

abort 'ERROR: missing hammer library' unless have_library 'hammer'
abort 'ERROR: missing hammer.h' unless have_header 'hammer.h'

create_makefile extension_name
