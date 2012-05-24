#ifndef HAMMER_DNS_RR__H
#define HAMMER_DNS_RR__H

#include "../src/hammer.h"

const parser_t* init_cname();
const parser_t* init_hinfo();
const parser_t* init_mb();
const parser_t* init_md();
const parser_t* init_mf();
const parser_t* init_mg();
const parser_t* init_minfo();
const parser_t* init_mr();
const parser_t* init_mx();
const parser_t* init_null();
const parser_t* init_ns();
const parser_t* init_ptr();
const parser_t* init_soa();
const parser_t* init_txt();
const parser_t* init_a();
const parser_t* init_wks();

#endif
