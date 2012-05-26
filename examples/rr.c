#include "../src/hammer.h"
#include "dns_common.h"
#include "rr.h"

#define false 0
#define true 1

const HParser* init_cname() {
  static const HParser *cname = NULL;
  if (cname)
    return cname;
  
  cname = h_sequence(init_domain(),
		     h_end_p(),
		     NULL);
  
  return cname;
}

const HParser* init_hinfo() {
  static const HParser *hinfo = NULL;
  if (hinfo)
    return hinfo;

  const HParser* cstr = init_character_string();
  
  hinfo = h_sequence(cstr,
		     cstr,
		     h_end_p(),
		     NULL);

  return hinfo;
}

const HParser* init_mb() {
  static const HParser *mb = NULL;
  if (mb)
    return mb;
  
  mb = h_sequence(init_domain(),
		  h_end_p(),
		  NULL);

  return mb;
}

const HParser* init_md() {
  static const HParser *md = NULL;
  if (md)
    return md;
  
  md = h_sequence(init_domain(),
		  h_end_p,
		  NULL);

  return md;
}

const HParser* init_mf() {
  static const HParser *mf = NULL;
  if (mf)
    return mf;
  
  mf = h_sequence(init_domain(),
		  h_end_p(),
		  NULL);

  return mf;
}

const HParser* init_mg() {
  static const HParser *mg = NULL;
  if (mg)
    return mg;
  
  mg = h_sequence(init_domain(),
		  h_end_p(),
		  NULL);

  return mg;
}

const HParser* init_minfo() {
  static const HParser *minfo = NULL;
  if (minfo)
    return minfo;

  const HParser* domain = init_domain();

  minfo = h_sequence(domain,
		     domain,
		     h_end_p(),
		     NULL);

  return minfo;
}

const HParser* init_mr() {
  static const HParser *mr = NULL;
  if (mr)
    return mr;
  
  mr = h_sequence(init_domain(),
		  h_end_p(),
		  NULL);

  return mr;
}

const HParser* init_mx() {
  static const HParser *mx = NULL;
  if (mx)
    return mx;
  
  mx = h_sequence(h_uint16(),
		  init_domain(),
		  h_end_p(),
		  NULL);

  return mx;
}

bool validate_null(HParseResult *p) {
  if (TT_SEQUENCE != p->ast->token_type)
    return false;
  return (65536 > p->ast->seq->used);
}

const HParser* init_null() {
  static const HParser *null_ = NULL;
  if (null_)
    return null_;

  null_ = h_attr_bool(h_many(h_uint8()), validate_null);

  return null_;
}

const HParser* init_ns() {
  static const HParser *ns = NULL;
  if (ns)
    return ns;

  ns = h_sequence(init_domain(),
		  h_end_p(),
		  NULL);

  return ns;
}

const HParser* init_ptr() {
  static const HParser *ptr = NULL;
  if (ptr)
    return ptr;
  
  ptr = h_sequence(init_domain(),
		   h_end_p(),
		   NULL);

  return ptr;
}

const HParser* init_soa() {
  static const HParser *soa = NULL;
  if (soa)
    return soa;

  const HParser *domain = init_domain();

  soa = h_sequence(domain,   // MNAME
		   domain,   // RNAME
		   h_uint32(), // SERIAL
		   h_uint32(), // REFRESH
		   h_uint32(), // RETRY
		   h_uint32(), // EXPIRE
		   h_uint32(), // MINIMUM
		   h_end_p(),
		   NULL);

  return soa;
}

const HParser* init_txt() {
  static const HParser *txt = NULL;
  if (txt)
    return txt;

  txt = h_sequence(h_many1(init_character_string()),
		   h_end_p(),
		   NULL);

  return txt;
}

const HParser* init_a() {
  static const HParser *a = NULL;
  if (a)
    return a;

  a = h_sequence(h_uint32(),
		 h_end_p(),
		 NULL);

  return a;
}

const HParser* init_wks() {
  static const HParser *wks = NULL;
  if (wks)
    return wks;

  wks = h_sequence(h_uint32(),
		   h_uint8(),
		   h_many(h_uint8()),
		   h_end_p(),
		   NULL);

  return wks;
}
