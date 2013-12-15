require 'ffi'

module Hammer
  module Internal
    extend FFI::Library

    ffi_lib 'libhammer.dylib'

    typedef :pointer, :h_parser
    typedef :pointer, :h_parse_result

    # run a parser
    attach_function :h_parse, [:h_parser, :string, :size_t], :h_parse_result

    # build a parser
    attach_function :h_token, [:string, :size_t], :h_parser
    attach_function :h_ch, [:uint8], :h_parser
    attach_function :h_ch_range, [:uint8, :uint8], :h_parser
    attach_function :h_int_range, [:int64, :int64], :h_parser
    attach_function :h_bits, [:size_t, :bool], :h_parser
    attach_function :h_int64, [], :h_parser
    attach_function :h_int32, [], :h_parser
    attach_function :h_int16, [], :h_parser
    attach_function :h_int8,  [], :h_parser
    attach_function :h_uint64, [], :h_parser
    attach_function :h_uint32, [], :h_parser
    attach_function :h_uint16, [], :h_parser
    attach_function :h_uint8,  [], :h_parser
    attach_function :h_whitespace, [:h_parser], :h_parser
    attach_function :h_left,   [:h_parser, :h_parser], :h_parser
    attach_function :h_right,  [:h_parser, :h_parser], :h_parser
    attach_function :h_middle, [:h_parser, :h_parser, :h_parser], :h_parser
    #attach_function :h_in, [:string, :size_t], :h_parser
    #attach_function :h_not_in, [:string, :size_t], :h_parser
    attach_function :h_end_p, [], :h_parser
    attach_function :h_nothing_p, [], :h_parser
    attach_function :h_sequence, [:varargs], :h_parser
    attach_function :h_choice, [:varargs], :h_parser
    attach_function :h_butnot, [:h_parser, :h_parser], :h_parser
    attach_function :h_difference, [:h_parser, :h_parser], :h_parser
    attach_function :h_xor, [:h_parser, :h_parser], :h_parser
    attach_function :h_many, [:h_parser], :h_parser
    attach_function :h_many1, [:h_parser], :h_parser
    #attach_function :h_repeat_n, [:h_parser, :size_t], :h_parser
    attach_function :h_optional, [:h_parser], :h_parser
    attach_function :h_ignore, [:h_parser], :h_parser
    attach_function :h_sepBy, [:h_parser, :h_parser], :h_parser
    attach_function :h_sepBy1, [:h_parser, :h_parser], :h_parser
    attach_function :h_epsilon_p, [], :h_parser
    attach_function :h_length_value, [:h_parser, :h_parser], :h_parser
    attach_function :h_and, [:h_parser], :h_parser
    attach_function :h_not, [:h_parser], :h_parser

    attach_function :h_indirect, [], :h_parser
    attach_function :h_bind_indirect, [:h_parser, :h_parser], :void

    #attach_function :h_action, [:h_parser, ...], :h_parser
    #attach_function :h_attr_bool, [:h_parser, ...], :h_parser

    #class HParseResult < FFI::Struct
    #  layout  :ast, :pointer,
    #          :bit_length, :longlong,
    #          :arena, :pointer
    #end

    # free the parse result
    attach_function :h_parse_result_free, [:h_parse_result], :void

    # TODO: Does the HParser* need to be freed?
  end
end
