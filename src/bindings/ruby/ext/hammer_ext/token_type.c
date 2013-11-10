#include <ruby.h>
#include <hammer.h>

#include "token_type.h"

#define DefineHammerInternalConst(name) rb_define_const(mHammerInternal, #name, INT2FIX(name));

void Init_token_type(void)
{
  VALUE mHammer = rb_define_module("Hammer");
  VALUE mHammerInternal = rb_define_module_under(mHammer, "Internal");

  DefineHammerInternalConst(TT_NONE);
  DefineHammerInternalConst(TT_BYTES);
  DefineHammerInternalConst(TT_SINT);
  DefineHammerInternalConst(TT_UINT);
  DefineHammerInternalConst(TT_SEQUENCE);
  DefineHammerInternalConst(TT_ERR);
  DefineHammerInternalConst(TT_USER);
}
