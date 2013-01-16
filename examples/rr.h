#ifndef HAMMER_DNS_RR__H
#define HAMMER_DNS_RR__H

#include "../src/hammer.h"

const HParser* init_rdata(uint16_t type);

const HParser* init_cname();
const HParser* init_hinfo();
const HParser* init_mb();
const HParser* init_md();
const HParser* init_mf();
const HParser* init_mg();
const HParser* init_minfo();
const HParser* init_mr();
const HParser* init_mx();
const HParser* init_null();
const HParser* init_ns();
const HParser* init_ptr();
const HParser* init_soa();
const HParser* init_txt();
const HParser* init_a();
const HParser* init_wks();

#endif
