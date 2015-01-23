#include "parser_internal.h"

typedef struct {
    const HParser *p;
    HContinuation k;
    void *env;
} BindEnv;

static HParseResult *parse_bind(void *be_, HParseState *state) {
    BindEnv *be = be_;

    HParseResult *res = h_do_parse(be->p, state);
    if(!res)
        return NULL;

    HParser *kx = be->k(res->ast, be->env);
    return h_do_parse(kx, state);
}

static const HParserVtable bind_vt = {
    .parse = parse_bind,
    .isValidRegular = h_false,
    .isValidCF = h_false,
    .compile_to_rvm = h_not_regular,
};

HParser *h_bind(const HParser *p, HContinuation k, void *env)
{
    return h_bind__m(&system_allocator, p, k, env);
}

HParser *h_bind__m(HAllocator *mm__,
                   const HParser *p, HContinuation k, void *env)
{
    BindEnv *be = h_new(BindEnv, 1);

    be->p = p;
    be->k = k;
    be->env = env;

    return h_new_parser(mm__, &bind_vt, be);
}
