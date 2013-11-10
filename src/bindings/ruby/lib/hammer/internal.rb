require 'ffi'

module Hammer
  module Internal
    extend FFI::Library

    ffi_lib 'libhammer.dylib'

    # run a parser
    attach_function :h_parse, [:pointer, :string, :size_t], :pointer

    # build a parser
    attach_function :h_token, [:string, :size_t], :pointer
    attach_function :h_ch, [:uint8], :pointer
    attach_function :h_ch_range, [:uint8, :uint8], :pointer
    attach_function :h_int_range, [:int64, :int64], :pointer
    attach_function :h_bits, [:size_t, :bool], :pointer
    attach_function :h_int64, [], :pointer
    attach_function :h_int32, [], :pointer
    attach_function :h_int16, [], :pointer
    attach_function :h_int8,  [], :pointer
    attach_function :h_uint64, [], :pointer
    attach_function :h_uint32, [], :pointer
    attach_function :h_uint16, [], :pointer
    attach_function :h_uint8,  [], :pointer
    attach_function :h_whitespace, [:pointer], :pointer
    attach_function :h_left,   [:pointer, :pointer], :pointer
    attach_function :h_right,  [:pointer, :pointer], :pointer
    attach_function :h_middle, [:pointer, :pointer, :pointer], :pointer
    # h_action
    # h_in
    # h_not_in
    attach_function :h_end_p, [], :pointer
    attach_function :h_nothing_p, [], :pointer
    attach_function :h_sequence, [:varargs], :pointer
    attach_function :h_choice, [:varargs], :pointer
    attach_function :h_butnot, [:pointer, :pointer], :pointer
    attach_function :h_difference, [:pointer, :pointer], :pointer
    attach_function :h_xor, [:pointer, :pointer], :pointer
    attach_function :h_many, [:pointer], :pointer
    attach_function :h_many1, [:pointer], :pointer
    # h_repeat_n
    # h_optional
    # h_ignore
    # h_sepBy
    # h_sepBy1
    # h_epsilon_p
    # h_length_value
    # h_attr_bool
    # h_and
    # h_not
    # h_indirect
    # h_bind_indirect

    # free the parse result
    # h_parse_result_free

    # TODO: Does the HParser* need to be freed?
  end
end
