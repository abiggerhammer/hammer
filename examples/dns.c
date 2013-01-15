#include <sys/socket.h>
#include <netinet/in.h>
#include <err.h>
#include <string.h>
#include <assert.h>
#include "../src/hammer.h"
#include "dns_common.h"
#include "dns.h"
#include "rr.h"

#define false 0
#define true 1

#define H_RULE(rule, def) const HParser *rule = def
#define H_ARULE(rule, def) const HParser *rule = h_action(def, act_ ## rule)


bool is_zero(HParseResult *p) {
  if (TT_UINT != p->ast->token_type)
    return false;
  return (0 == p->ast->uint);
}

/**
 * Every DNS message should have QDCOUNT entries in the question
 * section, and ANCOUNT+NSCOUNT+ARCOUNT resource records.
 */
bool validate_dns(HParseResult *p) {
  if (TT_SEQUENCE != p->ast->token_type)
    return false;
  assert(p->ast->seq->elements[0]->token_type == (HTokenType)TT_dns_header);
  dns_header_t *header = (dns_header_t *)p->ast->seq->elements[0]->user;
  size_t qd = header->question_count;
  size_t an = header->answer_count;
  size_t ns = header->authority_count;
  size_t ar = header->additional_count;
  HParsedToken *questions = p->ast->seq->elements[1];
  if (questions->seq->used != qd)
    return false;
  HParsedToken *rrs = p->ast->seq->elements[2];
  if (an+ns+ar != rrs->seq->used)
    return false;
  return true;
}

struct dns_qname get_qname(const HParsedToken *t) {
  // The qname parser parses at least 1 length-value pair, then a NULL.
  // So, t->seq->elements[0] is a sequence of at least 1 such pair,
  // and t->seq->elements[1] is the null.
  const HParsedToken *labels = t->seq->elements[0];
  struct dns_qname ret = {
    .qlen = labels->seq->used,
    .labels = h_arena_malloc(t->seq->arena, sizeof(*ret.labels)*labels->seq->used)
  };
  // i is which label we're on
  for (size_t i=0; i<labels->seq->used; ++i) {
    ret.labels[i].len = labels->seq->elements[i]->seq->used;
    ret.labels[i].label = h_arena_malloc(t->seq->arena, ret.labels[i].len + 1);
    // j is which char of the label we're on
    for (size_t j=0; j<ret.labels[i].len; ++j)
      ret.labels[i].label[j] = labels->seq->elements[i]->seq->elements[j]->uint;
    ret.labels[i].label[ret.labels[i].len] = 0;
  }
  return ret;
}

char* get_domain(const HParsedToken *t) {
  switch(t->token_type) {
  case TT_UINT:
    return " ";
  case TT_SEQUENCE:
    {
      // Sequence of subdomains separated by "."
      // Each subdomain is a label, which can be no more than 63 chars.
      char *ret = h_arena_malloc(t->seq->arena, 64*t->seq->used);
      size_t count = 0;
      for (size_t i=0; i<t->seq->used; ++i) {
	HParsedToken *tmp = t->seq->elements[i];
	for (size_t j=0; j<tmp->seq->used; ++j) {
	  ret[count] = tmp->seq->elements[i]->uint;
	  ++count;
	}
	ret[count] = '.';
	++count;
      }
      ret[count-1] = '\x00';
      return ret;
    }
  default:
    return NULL;
  }
}

uint8_t* get_cs(const HCountedArray *arr) {
  uint8_t *ret = h_arena_malloc(arr->arena, sizeof(uint8_t)*arr->used);
  for (size_t i=0; i<arr->used; ++i)
    ret[i] = arr->elements[i]->uint;
  return ret;
}

uint8_t** get_txt(const HCountedArray *arr) {
  uint8_t **ret = h_arena_malloc(arr->arena, sizeof(uint8_t*)*arr->used);
  for (size_t i=0; i<arr->used; ++i) {
    uint8_t *tmp = h_arena_malloc(arr->arena, sizeof(uint8_t)*arr->elements[i]->seq->used);
    for (size_t j=0; j<arr->elements[i]->seq->used; ++j)
      tmp[j] = arr->elements[i]->seq->elements[j]->uint;
  }
  return ret;
}

void set_rr(struct dns_rr rr, HCountedArray *rdata) {
  uint8_t *data = h_arena_malloc(rdata->arena, sizeof(uint8_t)*rdata->used);
  for (size_t i=0; i<rdata->used; ++i)
    data[i] = rdata->elements[i]->uint;

  // If the RR doesn't parse, set its type to 0.
  switch(rr.type) {
  case 1: // A
    {
      const HParseResult *r = h_parse(init_a(), (const uint8_t*)data, rdata->used);
      if (!r) 
	rr.type = 0;
      else 
	rr.a = r->ast->seq->elements[0]->uint;
      break;
    }
  case 2: // NS
    {
      const HParseResult *r = h_parse(init_ns(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else
	rr.ns = get_domain(r->ast->seq->elements[0]);
      break;
    }
  case 3: // MD
    {
      const HParseResult *r = h_parse(init_md(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else
	rr.md = get_domain(r->ast->seq->elements[0]);
      break;
    }
  case 4: // MF
    {
      const HParseResult *r = h_parse(init_mf(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else
	rr.md = get_domain(r->ast->seq->elements[0]);
      break;
    }
  case 5: // CNAME
    {
      const HParseResult *r = h_parse(init_cname(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else
	rr.cname = get_domain(r->ast->seq->elements[0]);
      break;
    }
  case 6: // SOA
    {
      const HParseResult *r = h_parse(init_soa(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else {
	rr.soa.mname = get_domain(r->ast->seq->elements[0]);
	rr.soa.rname = get_domain(r->ast->seq->elements[1]);
	rr.soa.serial = r->ast->seq->elements[2]->uint;
	rr.soa.refresh = r->ast->seq->elements[3]->uint;
	rr.soa.retry = r->ast->seq->elements[4]->uint;
	rr.soa.expire = r->ast->seq->elements[5]->uint;
	rr.soa.minimum = r->ast->seq->elements[6]->uint;
      }
      break;
    }
  case 7: // MB
    {
      const HParseResult *r = h_parse(init_mb(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else
	rr.mb = get_domain(r->ast->seq->elements[0]);
      break;
    }
  case 8: // MG
    {
      const HParseResult *r = h_parse(init_mg(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else
	rr.mg = get_domain(r->ast->seq->elements[0]);
      break;
    }
  case 9: // MR
    {
      const HParseResult *r = h_parse(init_mr(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else
	rr.mr = get_domain(r->ast->seq->elements[0]);
      break;
    }
  case 10: // NULL
    {
      const HParseResult *r = h_parse(init_null(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else {
	rr.null = h_arena_malloc(rdata->arena, sizeof(uint8_t)*r->ast->seq->used);
	for (size_t i=0; i<r->ast->seq->used; ++i)
	  rr.null[i] = r->ast->seq->elements[i]->uint;
      }
      break;
    }
  case 11: // WKS
    {
      const HParseResult *r = h_parse(init_wks(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else {
	rr.wks.address = r->ast->seq->elements[0]->uint;
	rr.wks.protocol = r->ast->seq->elements[1]->uint;
	rr.wks.len = r->ast->seq->elements[2]->seq->used;
	rr.wks.bit_map = h_arena_malloc(rdata->arena, sizeof(uint8_t)*r->ast->seq->elements[2]->seq->used);
	for (size_t i=0; i<rr.wks.len; ++i)
	  rr.wks.bit_map[i] = r->ast->seq->elements[2]->seq->elements[i]->uint;
      }
      break;
    }
  case 12: // PTR
    {
      const HParseResult *r = h_parse(init_ptr(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else
	rr.ptr = get_domain(r->ast->seq->elements[0]);
      break;
    }
  case 13: // HINFO
    {
      const HParseResult *r = h_parse(init_hinfo(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else {
	rr.hinfo.cpu = get_cs(r->ast->seq->elements[0]->seq);
	rr.hinfo.os = get_cs(r->ast->seq->elements[1]->seq);
      }
      break;
    }
  case 14: // MINFO
    {
      const HParseResult *r = h_parse(init_minfo(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else {
	rr.minfo.rmailbx = get_domain(r->ast->seq->elements[0]);
	rr.minfo.emailbx = get_domain(r->ast->seq->elements[1]);
      }
      break;
    }
  case 15: // MX
    {
      const HParseResult *r = h_parse(init_mx(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else {
	rr.mx.preference = r->ast->seq->elements[0]->uint;
	rr.mx.exchange = get_domain(r->ast->seq->elements[1]);
      }
      break;
    }
  case 16: // TXT
    {
      const HParseResult *r = h_parse(init_txt(), (const uint8_t*)data, rdata->used);
      if (!r)
	rr.type = 0;
      else {
	rr.txt.count = r->ast->seq->elements[0]->seq->used;
	rr.txt.txt_data = get_txt(r->ast->seq->elements[0]->seq);
      }
      break;
    }
  default:
    break;
  }
}

const HParsedToken* act_header(const HParseResult *p) {
  HParsedToken *ret = h_arena_malloc(p->arena, sizeof(HParsedToken));
  ret->token_type = TT_dns_header;
  ret->user = h_arena_malloc(p->arena, sizeof(dns_header_t));

  HParsedToken **fields = p->ast->seq->elements;
  dns_header_t header_ = {
    .id     = fields[0]->uint,
    .qr     = fields[1]->uint,
    .opcode = fields[2]->uint,
    .aa     = fields[3]->uint,
    .tc     = fields[4]->uint,
    .rd     = fields[5]->uint,
    .ra     = fields[6]->uint,
    .rcode  = fields[7]->uint,
    .question_count   = fields[8]->uint,
    .answer_count     = fields[9]->uint,
    .authority_count  = fields[10]->uint,
    .additional_count = fields[11]->uint
  };
  *(dns_header_t *)ret->user = header_;

  return ret;
}

const HParsedToken* act_message(const HParseResult *p) {
  h_pprint(stdout, p->ast, 0, 2);
  HParsedToken *ret = h_arena_malloc(p->arena, sizeof(HParsedToken));
  ret->token_type = TT_dns_message;

  dns_message_t *msg = h_arena_malloc(p->arena, sizeof(dns_message_t));

  assert(p->ast->seq->elements[0]->token_type == (HTokenType)TT_dns_header);
  dns_header_t *header = (dns_header_t *)p->ast->seq->elements[0]->user;
  msg->header = *header;

  HParsedToken *qs = p->ast->seq->elements[1];
  struct dns_question *questions = h_arena_malloc(p->arena,
						sizeof(struct dns_question)*(header->question_count));
  for (size_t i=0; i<header->question_count; ++i) {
    // QNAME is a sequence of labels. In the parser, it's defined as
    // sequence(many1(length_value(...)), ch('\x00'), NULL).
    questions[i].qname = get_qname(qs->seq->elements[i]->seq->elements[0]);
    questions[i].qtype = qs->seq->elements[i]->seq->elements[1]->uint;
    questions[i].qclass = qs->seq->elements[i]->seq->elements[2]->uint;
  }
  msg->questions = questions;

  HParsedToken *rrs = p->ast->seq->elements[2];
  struct dns_rr *answers = h_arena_malloc(p->arena,
					  sizeof(struct dns_rr)*(header->answer_count));
  for (size_t i=0; i<header->answer_count; ++i) {
    answers[i].name = get_domain(rrs[i].seq->elements[0]);
    answers[i].type = rrs[i].seq->elements[1]->uint;
    answers[i].class = rrs[i].seq->elements[2]->uint;
    answers[i].ttl = rrs[i].seq->elements[3]->uint;
    answers[i].rdlength = rrs[i].seq->elements[4]->seq->used;
    set_rr(answers[i], rrs[i].seq->elements[4]->seq);	   
  }
  msg->answers = answers;

  struct dns_rr *authority = h_arena_malloc(p->arena,
					  sizeof(struct dns_rr)*(header->authority_count));
  for (size_t i=0, j=header->answer_count; i<header->authority_count; ++i, ++j) {
    authority[i].name = get_domain(rrs[j].seq->elements[0]);
    authority[i].type = rrs[j].seq->elements[1]->uint;
    authority[i].class = rrs[j].seq->elements[2]->uint;
    authority[i].ttl = rrs[j].seq->elements[3]->uint;
    authority[i].rdlength = rrs[j].seq->elements[4]->seq->used;
    set_rr(authority[i], rrs[j].seq->elements[4]->seq);
  }
  msg->authority = authority;

  struct dns_rr *additional = h_arena_malloc(p->arena,
					     sizeof(struct dns_rr)*(header->additional_count));
  for (size_t i=0, j=header->answer_count+header->authority_count; i<header->additional_count; ++i, ++j) {
    additional[i].name = get_domain(rrs[j].seq->elements[0]);
    additional[i].type = rrs[j].seq->elements[1]->uint;
    additional[i].class = rrs[j].seq->elements[2]->uint;
    additional[i].ttl = rrs[j].seq->elements[3]->uint;
    additional[i].rdlength = rrs[j].seq->elements[4]->seq->used;
    set_rr(additional[i], rrs[j].seq->elements[4]->seq);
  }
  msg->additional = additional;

  ret->user = (void*)msg;
  return ret;
}

// The action equivalent of h_ignore.
const HParsedToken *act_ignore(const HParseResult *p)
{
  return NULL;
}

#define act_hdzero act_ignore

const HParser* init_parser() {
  static const HParser *ret = NULL;
  if (ret)
    return ret;

  H_RULE (domain,   init_domain());
  H_ARULE(hdzero,   h_attr_bool(h_bits(3, false), is_zero));
  H_ARULE(header,   h_sequence(h_bits(16, false), // ID
			       h_bits(1, false),  // QR
			       h_bits(4, false),  // opcode
			       h_bits(1, false),  // AA
			       h_bits(1, false),  // TC
			       h_bits(1, false),  // RD
			       h_bits(1, false),  // RA
			       hdzero,            // Z
			       h_bits(4, false),  // RCODE
			       h_uint16(),        // QDCOUNT
			       h_uint16(),        // ANCOUNT
			       h_uint16(),        // NSCOUNT
			       h_uint16(),        // ARCOUNT
			       NULL));
  H_RULE (type,     h_int_range(h_uint16(), 1, 16));
  H_RULE (qtype,    h_choice(type, 
			     h_int_range(h_uint16(), 252, 255),
			     NULL));
  H_RULE (class,    h_int_range(h_uint16(), 1, 4));
  H_RULE (qclass,   h_choice(class,
			     h_int_range(h_uint16(), 255, 255),
			     NULL));
  H_RULE (len,      h_int_range(h_uint8(), 1, 255));
  H_RULE (label,    h_length_value(len, h_uint8())); 
  H_RULE (qname,    h_sequence(h_many1(label),
			       h_ch('\x00'),
			       NULL));
  H_RULE (question, h_sequence(qname, qtype, qclass, NULL));
  H_RULE (rdata,    h_length_value(h_uint16(), h_uint8()));
  H_RULE (rr,       h_sequence(domain,            // NAME
			       type,              // TYPE
			       class,             // CLASS
			       h_uint32(),        // TTL
			       rdata,             // RDLENGTH+RDATA
			       NULL));
  H_ARULE(message,  h_attr_bool(h_sequence(header,
					   h_many(question),
					   h_many(rr),
					   h_end_p(),
					   NULL),
			        validate_dns));

  ret = message;
  return ret;
}

int start_listening() {
  // return: fd
  int sock;
  struct sockaddr_in addr;

  sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    err(1, "Failed to open listning socket");
  addr.sin_family = AF_INET;
  addr.sin_port = htons(53);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int optval = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    err(1, "Bind failed");
  return sock;
}

const int TYPE_MAX = 16;
typedef const char* cstr;
const char* TYPE_STR[17] = {
  "nil", "A", "NS", "MD",
  "MF", "CNAME", "SOA", "MB",
  "MG", "MR", "NULL", "WKS",
  "PTR", "HINFO", "MINFO", "MX",
  "TXT"
};

const int CLASS_MAX = 4;
const char* CLASS_STR[5] = {
  "nil", "IN", "CS", "CH", "HS"
};


void format_qname(struct dns_qname *name, uint8_t **dest) {
  uint8_t *rp = *dest;
  for (size_t j = 0; j < name->qlen; j++) {
    *rp++ = name->labels[j].len;
    for (size_t k = 0; k < name->labels[j].len; k++)
      *rp++ = name->labels[j].label[k];
  }
  *rp++ = 0;
  *dest = rp;
}
      

int main(int argc, char** argv) {
  const HParser *parser = init_parser();
    
  
  // set up a listening socket...
  int sock = start_listening();

  uint8_t packet[8192]; // static buffer for simplicity
  ssize_t packet_size;
  struct sockaddr_in remote;
  socklen_t remote_len;

  
  while (1) {
    remote_len = sizeof(remote);
    packet_size = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr*)&remote, &remote_len);
    // dump the packet...
    for (int i = 0; i < packet_size; i++)
      printf(".%02hhx", packet[i]);
    
    printf("\n");

    HParseResult *content = h_parse(parser, packet, packet_size);
    if (!content) {
      printf("Invalid packet; ignoring\n");
      continue;
    }
    dns_message_t *message = content->ast->user;
    (void)message;
    for (size_t i = 0; i < message->header.question_count; i++) {
      struct dns_question *question = &message->questions[i];
      printf("Recieved %s %s request for ", CLASS_STR[question->qclass], TYPE_STR[question->qtype]);
      for (size_t j = 0; j < question->qname.qlen; j++)
	printf("%s.", question->qname.labels[j].label);
      printf("\n");
      
    }
    printf("%p\n", content);





    
    // Not much time to actually implement the DNS server for the talk, so here's something quick and dirty. 
    // Traditional response for this time of year...
    uint8_t response_buf[4096];
    uint8_t *rp = response_buf;
    // write out header...
    *rp++ = message->header.id >> 8;
    *rp++ = message->header.id & 0xff;
    *rp++ = 0x80 | (message->header.opcode << 3) | message->header.rd;
    *rp++ = 0x0; // change to 0 for no error...
    *rp++ = 0; *rp++ = 1; // QDCOUNT
    *rp++ = 0; *rp++ = 1; // ANCOUNT
    *rp++ = 0; *rp++ = 0; // NSCOUNT
    *rp++ = 0; *rp++ = 0; // ARCOUNT
    // encode the first question...
    {
      struct dns_question *question = &message->questions[0];
      format_qname(&question->qname, &rp);
      *rp++ = (question->qtype >> 8) & 0xff;
      *rp++ = (question->qtype     ) & 0xff;
      *rp++ = (question->qclass >> 8) & 0xff;
      *rp++ = (question->qclass     ) & 0xff;

      // it's a cname...
      format_qname(&question->qname, &rp);
      *rp++ = 0; *rp++ = 5;
      *rp++ = (question->qclass >> 8) & 0xff;
      *rp++ = (question->qclass     ) & 0xff;
      *rp++ = 0; *rp++ = 0; *rp++ = 0; *rp++ = 0; // TTL.
      //const char cname_rd[14] = "\x09spargelze\x02it";
      *rp++ = 0; *rp++ = 14;
      memcpy(rp, "\x09spargelze\x02it", 14);
      rp += 14;
    }
    // send response.
    sendto(sock, response_buf, (rp - response_buf), 0, (struct sockaddr*)&remote, remote_len);
  }
  return 0;
}


