#include "parser_internal.h"

typedef struct {
    const HParser *p;
    HContinuation k;
    void *env;
    HAllocator *mm__;
} BindEnv;

// an HAllocator backed by an HArena
typedef struct {
    HAllocator allocator;   // inherit  XXX is this the proper way to do it?
    HArena *arena;
} ArenaAllocator;

static void *aa_alloc(HAllocator *allocator, size_t size)
{
    HArena *arena = ((ArenaAllocator *)allocator)->arena;
    return h_arena_malloc(arena, size);
}

static void *aa_realloc(HAllocator *allocator, void *ptr, size_t size)
{
    HArena *arena = ((ArenaAllocator *)allocator)->arena;
    assert(((void)"XXX need realloc for arena allocator", 0));
    return NULL;
}

static void aa_free(HAllocator *allocator, void *ptr)
{
    HArena *arena = ((ArenaAllocator *)allocator)->arena;
    h_arena_free(arena, ptr);
}

static HParseResult *parse_bind(void *be_, HParseState *state) {
    BindEnv *be = be_;

    HParseResult *res = h_do_parse(be->p, state);
    if(!res)
        return NULL;

    // create a temporary arena allocator for the continuation
    HArena *arena = h_new_arena(be->mm__, 0);
    ArenaAllocator aa = {{aa_alloc, aa_realloc, aa_free}, arena};

    HParser *kx = be->k((HAllocator *)&aa, res->ast, be->env);
    if(!kx) {
        h_delete_arena(arena);
        return NULL;
    }

    res = h_do_parse(kx, state);

    h_delete_arena(arena);
    return res;
}

static const HParserVtable bind_vt = {
    .parse = parse_bind,
    .isValidRegular = h_false,
    .isValidCF = h_false,
    .compile_to_rvm = h_not_regular,
    .higher = true,
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
    be->mm__ = mm__;

    return h_new_parser(mm__, &bind_vt, be);
}
