%module hammer;

%include "stdint.i"

 // Special attention needs to be paid to:
 // h_parse
 // h_token
 // h_in
 // h_not_in

 //%typemap(cstype)  uint8_t* "byte[]"
%typemap(imtype) uint8_t* "IntPtr"
 //%typemap(csin, pre="unsafe { fixed(byte* temp$csinput = &$csinput[0]) {", terminator="}}") uint8_t* "(IntPtr)temp$csinput"
 //%typemap(csvarin) uint8_t
%typemap(cstype) uint8_t* "IntPtr"
%typemap(csin) uint8_t* "$csinput"
%typemap(csvarout) uint8_t* %{
    get {
      return $imcall;
    }
  %}

%ignore h_bit_writer_get_buffer;
%apply (char *STRING, size_t LENGTH) {(uint8_t* str, size_t len)};
%apply (uint8_t* str, size_t len) {(const uint8_t* input, size_t length)}
%apply (uint8_t* str, size_t len) {(const uint8_t* str, const size_t len)}
%apply (uint8_t* str, size_t len) {(const uint8_t* charset, size_t length)}

%typemap(csclassmodifiers) SWIGTYPE "internal class";
%csmethodmodifiers "internal";

%include "../swig/hammer.i";
