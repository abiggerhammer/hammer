#include "parser_internal.h"


typedef struct {
    const HParser *p;
    char endianness;
} HParseEndianness;

// helper
static void switch_bit_order(HInputStream *input)
{
    assert(input->bit_offset <= 8);

    char tmp = input->bit_offset;
    input->bit_offset = input->margin;
    input->margin = tmp;
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
    .higher = true,
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
