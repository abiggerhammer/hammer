Privileged arguments
====================

As a matter of convenience, there are several identifiers that
internal anaphoric macros use. Chances are that if you use these names
for other things, you're gonna have a bad time.

In particular, these names, and the macros that use them, are:
- state:
    Used by a_new and company. Should be an HParseState*
- mm__:
    Used by h_new and h_free. Should be an HAllocator*
- stk__:
    Used in desugaring. Should be an HCFStack*

Function suffixes
=================

Many functions come in several variants, to handle receiving optional
parameters or parameters in multiple different forms.  For example,
often, you have a global memory manager that is used for an entire
program. In this case, you can leave off the memory manager arguments
off, letting them be implicit instead. Further, it is often convenient
to pass an array or va_list to a function instead of listing the
arguments inline (eg, for wrapping a function, generating the
arguments programattically, or writing bindings for another language.

Because we have found that most variants fall into a fairly small set
of forms, and to minimize the amount of API calls that users need to
remember, there is a consistent naming scheme for these function
variants: the function name is followed by two underscores and a set
of single-character "flags" indicating what optional features that
particular variant has (in alphabetical order, of course):

  __a: takes variadic arguments as a void*[] (not implemented yet, but will be soon. 
  __m: takes a memory manager as the first argument, to override the system memory manager.
  __v: Takes the variadic argument list as a va_list


Memory managers
===============

If the __m function variants are used or system_allocator is
overridden, there come some difficult questions to answer,
particularly regarding the behavior when multiple memory managers are
combined. As a general rule of thumb (exceptions will be explicitly
documented), assume that

   If you have a function f, which is passed a memory manager m and
   returns a value r, any function that uses r as a parameter must
   also be told to use m as a memory manager.

In other words, don't let the (memory manager) streams cross.

Language-independent test suite
===============================

There is a language-independent representation of the Hammer test
suite in `lib/test-suite`.  This is intended to be used with the
tsparser.pl prolog library, along with a language-specific frontend.

Only the C# frontend exists so far; to regenerate the test suites using it, run

    $ swipl -q  -t halt -g tsgencsharp:prolog tsgencsharp.pl \
          >../src/bindings/dotnet/test/hammer_tests.cs

