require 'ffi'

module Hammer
  module Internal
    extend FFI::Library

    ffi_lib 'hammer'

    class DynamicVariable
      SYMBOL_PREFIX = "Hammer::Internal::DynamicVariable gensym "
      @@current_symbol = 0

      def initialize(default=nil, name=nil, &block)
        # This can take either a default value or a block.  If a
        # default value is given, all threads' dynvars are initialized
        # to that object.  If a block is given, the block is lazilly
        # called on each thread to generate the initial value.  If
        # both a block and a default value are passed, the block is
        # called with the literal value.
        @default = default
        @block = block || Proc.new{|x| x}
        @@current_symbol += 1
        @sym = (SYMBOL_PREFIX + @@current_symbol.to_s).to_sym
      end

      def value
        if Thread.current.key? @sym
          return Thread.current[@sym]
        else
          return Thread.current[@sym] = @block.call(@default)
        end
      end

      def value=(new_value)
        Thread.current[@sym] = new_value
      end

      def with(new_value, &block)
        old_value = value
        begin
          self.value = new_value
          return block.call
        ensure
          self.value = old_value
        end
      end
    end

    # Maybe we can implement Hammer::Parser with FFI::DataConverter.
    # That way, most hammer functions won't need to be wrapped.
    # (Probably need to wrap token, sequence and choice only).
    # See http://www.elabs.se/blog/61-advanced-topics-in-ruby-ffi
    typedef :pointer, :h_parser

    class HTokenType
      extend FFI::DataConverter

      @@known_type_map = {
        :none => 1,
        :bytes => 2,
        :sint => 4,
        :uint => 8,
        :sequence => 16,
      }

      @@inverse_type_map = @@known_type_map.invert

      @@from_hpt = {
        :none => Proc.new { nil },
        :bytes => Proc.new {|hpt| hpt[:data][:bytes].token},
        :sint => Proc.new {|hpt| hpt[:data][:sint]},
        :uint => Proc.new {|hpt| hpt[:data][:uint]},
        :sequence => Proc.new {|hpt| hpt[:data][:seq].map {|x| x.unmarshal}},
      }

      def self.new(name, &block)
        if name.is_a?(Symbol)
          name_sym = name
          name_str = name.to_s
        else
          name_str = name.to_s
          name_sym = name.to_sym
        end
        num = Hammer::Internal.h_allocate_token_type(name_str)
        @@known_type_map[name_sym] = num
        @@inverse_type_map[num] = name_sym
        @@from_hpt[name_sym] = block
      end

      def self.from_name(name)
        unless @@known_type_map.key? name
          num =  Hammer::Internal.h_get_token_type_number(name.to_s)
          if num <= 0
            raise ArgumentError, "Unknown token type #{name}"
          end
          @@known_type_map[name] = num
          @@inverse_type_map[num] = name
        end
        return @@known_type_map[name]
      end

      def self.from_num(num)
        unless @@inverse_type_map.key? num
          name = Hammer::Internal.h_get_token_type_name(num)
          if name.nil?
            return nil
          end
          name = name.to_sym
          @@known_type_map[name] = num
          @@inverse_type_map[num] = name
        end
        return @@inverse_type_map[num]
      end

      def self.native_type
        FFI::Type::INT
      end

      def self.to_native(val, ctx)
        return val if val.is_a?(Integer)
        return from_name(val)
      end

      def self.from_native(val, ctx)
        return from_num(val) || val
      end
    end

    # Define these as soon as possible, so that they can be used
    # without fear elsewhere
    attach_function :h_allocate_token_type, [:string], :int
    attach_function :h_get_token_type_number, [:string], :int
    attach_function :h_get_token_type_name, [:int], :string

    class HCountedArray < FFI::Struct
      layout  :capacity, :size_t,
              :used, :size_t,
              :arena, :pointer,
              :elements, :pointer # HParsedToken**

      def length
        self[:used]
      end

      def elements
        elem_array = FFI::Pointer.new(:pointer, self[:elements])
        return (0...self[:used]).map { |i| HParsedToken.new(elem_array[i].read_pointer) }
      end

      #def [](idx)
      #  raise ArgumentError, "Index out of range" unless idx >= 0 and idx < length
      #  elem_array = FFI::Pointer.new(:pointer, self[:elements])
      #  return HParsedToken.new(elem_array[i].read_pointer)
      #end

      def map(&code)
        elements.map {|x| code.call x}
      end
      def each(&code)
        elements.each {|x| code.call x}
      end
    end

    class HBytes < FFI::Struct
      layout  :token, :pointer, # uint8_t*
              :len, :size_t

      def token
        # TODO: Encoding?
        # Should be the same encoding as the string the token was created with.
        # But how do we get to this knowledge at this point?
        # Cheap solution: Just ask the user (additional parameter with default value of UTF-8).
        self[:token].read_string(self[:len])
      end

      # TODO: Probably should rename this to match ruby conventions: length, count, size
      def len
        self[:len]
      end
    end

    class HString < FFI::Struct
      layout :content, HBytes.by_ref,
             :encoding, :uint64
      def token
        return self[:content].token.force_encoding(
                  ObjectSpace._id2ref(self[:encoding]))
      end
    end

    HTokenType.new(:"com.upstandinghackers.hammer.ruby.encodedStr") {|hpt|
        hpt.user(HString).token
      }
    HTokenType.new(:"com.upstandinghackers.hammer.ruby.object") {|hpt|
        ObjectSpace._id2ref(hpt[:data][:uint])
      }

    class HParsedTokenDataUnion < FFI::Union
      layout  :bytes, HBytes.by_value,
              :sint, :int64,
              :uint, :uint64,
              :dbl, :double,
              :flt, :float,
              :seq, HCountedArray.by_ref,
              :user, :pointer
    end

    class HParsedToken < FFI::Struct
      layout  :token_type, HTokenType,
              :data, HParsedTokenDataUnion.by_value,
              :index, :size_t,
              :bit_offset, :char

      def normalize
        # If I'm null, return nil.
        return nil if null?
        return self
      end
      
      def token_type
        self[:token_type]
      end

      # TODO: Is this name ok?
      def data
        return self[:data][:bytes].token if token_type == :bytes
        return self[:data][:sint] if token_type == :sint
        return self[:data][:uint] if token_type == :uint
        return self[:data][:seq].elements if token_type == :sequence
        return self[:data][:user] if token_type == :user
      end

      def bytes
        raise ArgumentError, 'wrong token type' unless token_type == :bytes
        self[:data][:bytes]
      end

      def seq
        raise ArgumentError, 'wrong token type' unless token_type == :sequence
        self[:data][:seq]
      end

      def index
        self[:index]
      end

      def bit_offset
        self[:bit_offset]
      end

      def user(struct)
        struct.by_ref.from_native(self[:data][:user], nil)
      end

      def unmarshal
        Hammer::Internal::HTokenType.class_variable_get(:@@from_hpt)[token_type].call self
      end
    end

    class HParseResult < FFI::Struct
      layout  :ast, HParsedToken.by_ref,
              :bit_length, :long_long,
              :arena, :pointer

      def ast
        self[:ast].normalize
      end

      def bit_length
        self[:bit_length]
      end

      def self.release(ptr)
        Hammer::Internal.h_parse_result_free(ptr) unless ptr.null?
      end

      def arena_alloc(type)
        Hammer::Internal.arena_alloc(self[:arena], type)
      end
    end

    def self.arena_alloc(arena, type)
      ptr = h_arena_malloc(arena, type.size)
      return type.by_ref.from_native(ptr, nil)
    end

    # run a parser
    attach_function :h_parse, [:h_parser, :pointer, :size_t], HParseResult.auto_ptr # TODO: Use :buffer_in instead of :string?

    # build a parser
    attach_function :h_token, [:buffer_in, :size_t], :h_parser
    attach_function :h_ch, [:uint8], :h_parser
    attach_function :h_ch_range, [:uint8, :uint8], :h_parser
    attach_function :h_int_range, [:h_parser, :int64, :int64], :h_parser
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
    attach_function :h_in, [:pointer, :size_t], :h_parser
    attach_function :h_not_in, [:pointer, :size_t], :h_parser
    attach_function :h_end_p, [], :h_parser
    attach_function :h_nothing_p, [], :h_parser
    attach_function :h_sequence, [:varargs], :h_parser
    attach_function :h_choice, [:varargs], :h_parser
    attach_function :h_butnot, [:h_parser, :h_parser], :h_parser
    attach_function :h_difference, [:h_parser, :h_parser], :h_parser
    attach_function :h_xor, [:h_parser, :h_parser], :h_parser
    attach_function :h_many, [:h_parser], :h_parser
    attach_function :h_many1, [:h_parser], :h_parser
    attach_function :h_repeat_n, [:h_parser, :size_t], :h_parser
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

    callback :HAction, [HParseResult.by_ref], HParsedToken.by_ref
    attach_function :h_action, [:h_parser, :HAction], :h_parser

    callback :HPredicate, [HParseResult.by_ref], :bool
    attach_function :h_attr_bool, [:h_parser, :HPredicate], :h_parser

    # free the parse result
    attach_function :h_parse_result_free, [HParseResult.by_ref], :void

    # TODO: Does the HParser* need to be freed?

    # Add the arena
    attach_function :h_arena_malloc, [:pointer, :size_t], :pointer
  end
end
