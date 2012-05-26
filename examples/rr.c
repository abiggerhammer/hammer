#include "../src/hammer.h"
#include "dns_common.h"
#include "rr.h"

#define false 0
#define true 1

const parser_t* init_cname() {
  static const parser_t *cname = NULL;
  if (cname)
    return cname;
  
  cname = sequence(init_domain(),
		   end_p(),
		   NULL);
  
  return cname;
}

const parser_t* init_hinfo() {
  static const parser_t *hinfo = NULL;
  if (hinfo)
    return hinfo;

  const parser_t* cstr = init_character_string();
  
  hinfo = sequence(cstr,
		   cstr,
		   end_p(),
		   NULL);

  return hinfo;
}

const parser_t* init_mb() {
  static const parser_t *mb = NULL;
  if (mb)
    return mb;
  
  mb = sequence(init_domain(),
		end_p(),
		NULL);

  return mb;
}

const parser_t* init_md() {
  static const parser_t *md = NULL;
  if (md)
    return md;
  
  md = sequence(init_domain(),
		end_p,
		NULL);

  return md;
}

const parser_t* init_mf() {
  static const parser_t *mf = NULL;
  if (mf)
    return mf;
  
  mf = sequence(init_domain(),
		end_p(),
		NULL);

  return mf;
}

const parser_t* init_mg() {
  static const parser_t *mg = NULL;
  if (mg)
    return mg;
  
  mg = sequence(init_domain(),
		end_p(),
		NULL);

  return mg;
}

const parser_t* init_minfo() {
  static const parser_t *minfo = NULL;
  if (minfo)
    return minfo;

  const parser_t* domain = init_domain();

  minfo = sequence(domain,
		   domain,
		   end_p(),
		   NULL);

  return minfo;
}

const parser_t* init_mr() {
  static const parser_t *mr = NULL;
  if (mr)
    return mr;
  
  mr = sequence(init_domain(),
		end_p(),
		NULL);

  return mr;
}

const parser_t* init_mx() {
  static const parser_t *mx = NULL;
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

const parser_t* init_null() {
  static const parser_t *null_ = NULL;
  if (null_)
    return null_;

  null_ = attr_bool(uint8(), validate_null);

  return null_;
}

const parser_t* init_ns() {
  static const parser_t *ns = NULL;
  if (ns)
    return ns;

  ns = sequence(init_domain(),
		end_p(),
		NULL);

  return ns;
}

const parser_t* init_ptr() {
  static const parser_t *ptr = NULL;
  if (ptr)
    return ptr;
  
  ptr = sequence(init_domain(),
		end_p(),
		NULL);

  return ptr;
}

const parser_t* init_soa() {
  static const parser_t *soa = NULL;
  if (soa)
    return soa;

  const parser_t *domain = init_domain();

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

const parser_t* init_txt() {
  static const parser_t *txt = NULL;
  if (txt)
    return txt;

  txt = sequence(many1(init_character_string()),
		 end_p(),
		 NULL);

  return txt;
}

const parser_t* init_a() {
  static const parser_t *a = NULL;
  if (a)
    return a;

  a = sequence(uint32(),
	       end_p(),
	       NULL);

  return a;
}

const parser_t* init_wks() {
  static const parser_t *wks = NULL;
  if (wks)
    return wks;

  wks = sequence(uint32(),
		 uint8(),
		 many(uint8()),
		 end_p(),
		 NULL);

  return wks;
}
