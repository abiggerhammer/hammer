#include "parser_internal.h"


typedef struct {
    const HParser *p;
    char endianness;
} HParseEndianness;

// helper
static void switch_bit_order(HInputStream *input)
{
    assert(input->bit_offset <= 8);

    if((input->bit_offset % 8) != 0) {
        // switching bit order in the middle of a byte
        // we leave bit_offset untouched. this means that something like
        //     le(bits(5)),le(bits(3))
        // is equivalent to
        //     le(bits(5),bits(3)) .
        // on the other hand,
        //     le(bits(5)),be(bits(5))
        // will read the same 5 bits twice and discard the top 3.
    } else {
        // flip offset (0 <-> 8)
        input->bit_offset = 8 - input->bit_offset;
    }
}

static HParseResult *parse_endianness(void *env, HParseState *state)
{
    HParseEndianness *e = env;
    HParseResult *res = NULL;
    char diff = state->input_stream.endianness ^ e->endianness;

    if(!diff) {
        // all the same, nothing to do
        res = h_do_parse(e->p, state);
    } else {
        if(diff & BIT_BIG_ENDIAN)
            switch_bit_order(&state->input_stream);

        state->input_stream.endianness ^= diff;
        res = h_do_parse(e->p, state);
        state->input_stream.endianness ^= diff;

        if(diff & BIT_BIG_ENDIAN)
            switch_bit_order(&state->input_stream);
    }

    return res;
}

static const HParserVtable endianness_vt = {
    .parse = parse_endianness,
    .isValidRegular = h_false,
    .isValidCF = h_false,
    .desugar = NULL,
    .compile_to_rvm = h_not_regular,
};

HParser* h_with_endianness(char endianness, const HParser *p)
{
    return h_with_endianness__m(&system_allocator, endianness, p);
}

HParser* h_with_endianness__m(HAllocator *mm__, char endianness, const HParser *p)
{
    HParseEndianness *env = h_new(HParseEndianness, 1);
    env->endianness = endianness;
    env->p = p;
    return h_new_parser(mm__, &endianness_vt, env);
}
