%module hammer;

%include "stdint.i"

 // Special attention needs to be paid to:
 // h_parse
 // h_token
 // h_in
 // h_not_in

 //%typemap(cstype)  uint8_t* "byte[]"
 //%typemap(csin, pre="unsafe { fixed(byte* temp$csinput = &$csinput[0]) {", terminator="}}") uint8_t* "(IntPtr)temp$csinput"
 //%typemap(csvarin) uint8_t
%typemap(imtype) uint8_t* "IntPtr"
%typemap(cstype) uint8_t* "IntPtr"
%typemap(csin) uint8_t* "$csinput"
%typemap(csvarout) uint8_t* %{
    get {
      return $imcall;
    }
  %}

%typemap(imtype) void*[] "IntPtr"
%typemap(cstype) void*[] "IntPtr"
%typemap(csin) void*[] "$csinput"

%ignore h_bit_writer_get_buffer;
//%apply (char *STRING, size_t LENGTH) {(uint8_t* str, size_t len)};
//%apply (uint8_t* str, size_t len) {(const uint8_t* input, size_t length)}
//%apply (uint8_t* str, size_t len) {(const uint8_t* str, const size_t len)}
//%apply (uint8_t* str, size_t len) {(const uint8_t* charset, size_t length)}

%typemap(csclassmodifiers) SWIGTYPE "internal class";
%csmethodmodifiers "internal";

%extend HCountedArray_ {
  HParsedToken* at(unsigned int posn) {
    if (posn >= $self->used)
      return NULL;
    return $self->elements[posn];
  }
 }

%include "../swig/hammer.i";

%{
HTokenType dotnet_tagged_token_type;
 %}
%inline {
  void h_set_dotnet_tagged_token_type(HTokenType new_tt) {
    dotnet_tagged_token_type = new_tt;
  }
  // Need this inline as well
  struct HTaggedToken {
    HParsedToken *token;
    uint64_t label;
  };

// this is to make it easier to access via SWIG
struct HTaggedToken *h_parsed_token_get_tagged_token(HParsedToken* hpt) {
  return (struct HTaggedToken*)hpt->token_data.user;
}

HParsedToken *act_tag(const HParseResult* p, void* user_data) {
  struct HTaggedToken *tagged = H_ALLOC(struct HTaggedToken);
  tagged->label = *(uint64_t*)user_data;
  tagged->token = p->ast;
  return h_make(p->arena, dotnet_tagged_token_type, tagged);
}

HParser *h_tag__m(HAllocator *mm__, HParser *p, uint64_t tag) {
  uint64_t *tagptr = h_new(uint64_t, 1);
  *tagptr = tag;
  return h_action__m(mm__, p, act_tag, tagptr);
}

HParser *h_tag(HParser *p, uint64_t tag) {
  return h_tag__m(&system_allocator, p, tag);
}
 }
