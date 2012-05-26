#include "../src/hammer.h"
#include "dns_common.h"
#include "rr.h"

#define false 0
#define true 1

const HParser* init_cname() {
  static const HParser *cname = NULL;
  if (cname)
    return cname;
  
  cname = sequence(init_domain(),
		   end_p(),
		   NULL);
  
  return cname;
}

const HParser* init_hinfo() {
  static const HParser *hinfo = NULL;
  if (hinfo)
    return hinfo;

  const HParser* cstr = init_character_string();
  
  hinfo = sequence(cstr,
		   cstr,
		   end_p(),
		   NULL);

  return hinfo;
}

const HParser* init_mb() {
  static const HParser *mb = NULL;
  if (mb)
    return mb;
  
  mb = sequence(init_domain(),
		end_p(),
		NULL);

  return mb;
}

const HParser* init_md() {
  static const HParser *md = NULL;
  if (md)
    return md;
  
  md = sequence(init_domain(),
		end_p,
		NULL);

  return md;
}

const HParser* init_mf() {
  static const HParser *mf = NULL;
  if (mf)
    return mf;
  
  mf = sequence(init_domain(),
		end_p(),
		NULL);

  return mf;
}

const HParser* init_mg() {
  static const HParser *mg = NULL;
  if (mg)
    return mg;
  
  mg = sequence(init_domain(),
		end_p(),
		NULL);

  return mg;
}

const HParser* init_minfo() {
  static const HParser *minfo = NULL;
  if (minfo)
    return minfo;

  const HParser* domain = init_domain();

  minfo = sequence(domain,
		   domain,
		   end_p(),
		   NULL);

  return minfo;
}

const HParser* init_mr() {
  static const HParser *mr = NULL;
  if (mr)
    return mr;
  
  mr = sequence(init_domain(),
		end_p(),
		NULL);

  return mr;
}

const HParser* init_mx() {
  static const HParser *mx = NULL;
  if (mx)
    return mx;
  
  mx = sequence(uint16(),
		init_domain(),
		end_p(),
		NULL);

  return mx;
}

bool validate_null(parse_result_t *p) {
  if (TT_SEQUENCE != p->ast->token_type)
    return false;
  return (65536 > p->ast->seq->used);
}

const HParser* init_null() {
  static const HParser *null_ = NULL;
  if (null_)
    return null_;

  null_ = attr_bool(uint8(), validate_null);

  return null_;
}

const HParser* init_ns() {
  static const HParser *ns = NULL;
  if (ns)
    return ns;

  ns = sequence(init_domain(),
		end_p(),
		NULL);

  return ns;
}

const HParser* init_ptr() {
  static const HParser *ptr = NULL;
  if (ptr)
    return ptr;
  
  ptr = sequence(init_domain(),
		end_p(),
		NULL);

  return ptr;
}

const HParser* init_soa() {
  static const HParser *soa = NULL;
  if (soa)
    return soa;

  const HParser *domain = init_domain();

  soa = sequence(domain,   // MNAME
		 domain,   // RNAME
		 uint32(), // SERIAL
		 uint32(), // REFRESH
		 uint32(), // RETRY
		 uint32(), // EXPIRE
		 uint32(), // MINIMUM
		 end_p(),
		 NULL);

  return soa;
}

const HParser* init_txt() {
  static const HParser *txt = NULL;
  if (txt)
    return txt;

  txt = sequence(many1(init_character_string()),
		 end_p(),
		 NULL);

  return txt;
}

const HParser* init_a() {
  static const HParser *a = NULL;
  if (a)
    return a;

  a = sequence(uint32(),
	       end_p(),
	       NULL);

  return a;
}

const HParser* init_wks() {
  static const HParser *wks = NULL;
  if (wks)
    return wks;

  wks = sequence(uint32(),
		 uint8(),
		 many(uint8()),
		 end_p(),
		 NULL);

  return wks;
}
