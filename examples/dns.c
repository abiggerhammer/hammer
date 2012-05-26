#include "../src/hammer.h"
#include "dns_common.h"
#include "dns.h"

#define false 0
#define true 1

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
  // The header holds the counts as its last 4 elements.
  HParsedToken **elems = p->ast->seq->elements[0]->seq->elements;
  size_t qd = elems[8]->uint;
  size_t an = elems[9]->uint;
  size_t ns = elems[10]->uint;
  size_t ar = elems[11]->uint;
  HParsedToken *questions = p->ast->seq->elements[1];
  if (questions->seq->used != qd)
    return false;
  HParsedToken *rrs = p->ast->seq->elements[2];
  if (an+ns+ar != rrs->seq->used)
    return false;
  return true;
}



const HParsedToken* pack_dns_struct(const HParseResult *p) {
  HParsedToken *ret = arena_malloc(p->arena, sizeof(HParsedToken*));
  ret->token_type = TT_USER;

  dns_message_t *msg = arena_malloc(p->arena, sizeof(dns_message_t*));

  HParsedToken *hdr = p->ast->seq->elements[0];
  struct dns_header header = {
    .id = hdr->seq->elements[0]->uint,
    .qr = hdr->seq->elements[1]->uint,
    .opcode = hdr->seq->elements[2]->uint,
    .aa = hdr->seq->elements[3]->uint,
    .tc = hdr->seq->elements[4]->uint,
    .rd = hdr->seq->elements[5]->uint,
    .ra = hdr->seq->elements[6]->uint,
    .rcode = hdr->seq->elements[7]->uint,
    .question_count = hdr->seq->elements[8]->uint,
    .answer_count = hdr->seq->elements[9]->uint,
    .authority_count = hdr->seq->elements[10]->uint,
    .additional_count = hdr->seq->elements[11]->uint
  };
  msg->header = header;

  HParsedToken *qs = p->ast->seq->elements[1];
  struct dns_question *questions = arena_malloc(p->arena,
						sizeof(struct dns_question)*(header.question_count));
  for (size_t i=0; i<header.question_count; ++i) {
    questions[i].qname = get_qname(qs->seq->elements[i]->seq->elements[0]);
    questions[i].qtype = qs->seq->elements[i]->seq->elements[1]->uint;
    questions[i].qclass = qs->seq->elements[i]->seq->elements[2]->uint;
  }
  msg->questions = questions;

  HParsedToken *rrs = p->ast->seq->elements[2];
  struct dns_rr *answers = arena_malloc(p->arena,
					sizeof(struct dns_rr)*(header.answer_count));
  for (size_t i=0; i<header.answer_count; ++i) {
    answers[i].name = get_domain(rrs[i].seq->elements[0]);
    answers[i].type = rrs[i].seq->elements[1]->uint;
    answers[i].type = rrs[i].seq->elements[2]->uint;
    answers[i].ttl = rrs[i].seq->elements[3]->uint;
    answers[i].rdlength = rrs[i].seq->elements[4]->seq->used;
    set_rr(answers[i], rrs[i].seq->elements[4]->seq);	   
  }
  msg->answers = answers;

  struct dns_rr *authority = arena_malloc(p->arena,
					  sizeof(struct dns_rr)*(header.authority_count));
  for (size_t i=0, j=header.answer_count; i<header.authority_count; ++i, ++j) {
    authority[i].name = get_domain(rrs[j].seq->elements[0]);
    authority[i].type = rrs[j].seq->elements[1]->uint;
    authority[i].type = rrs[j].seq->elements[2]->uint;
    authority[i].ttl = rrs[j].seq->elements[3]->uint;
    authority[i].rdlength = rrs[j].seq->elements[4]->seq->used;
    set_rr(authority[i], rrs[j].seq->elements[4]->seq);
  }
  msg->authority = authority;

  struct dns_rr *additional = arena_malloc(p->arena,
					   sizeof(struct dns_rr)*(header.additional_count));
  for (size_t i=0, j=header.answer_count+header.authority_count; i<header.additional_count; ++i, ++j) {
    additional[i].name = get_domain(rrs[j].seq->elements[0]);
    additional[i].type = rrs[j].seq->elements[1]->uint;
    additional[i].type = rrs[j].seq->elements[2]->uint;
    additional[i].ttl = rrs[j].seq->elements[3]->uint;
    additional[i].rdlength = rrs[j].seq->elements[4]->seq->used;
    set_rr(additional[i], rrs[j].seq->elements[4]->seq);
  }
  msg->additional = additional;

  ret->user = (void*)msg;
  return ret;
}

const HParser* init_parser() {
  static HParser *dns_message = NULL;
  if (dns_message)
    return dns_message;

  const HParser *domain = init_domain();

  const HParser *dns_header = sequence(bits(16, false), // ID
					bits(1, false),  // QR
					bits(4, false),  // opcode
					bits(1, false),  // AA
					bits(1, false),  // TC
					bits(1, false),  // RD
					bits(1, false),  // RA
					ignore(attr_bool(bits(3, false), is_zero)), // Z
					bits(4, false),  // RCODE
					uint16(), // QDCOUNT
					uint16(), // ANCOUNT
					uint16(), // NSCOUNT
					uint16(), // ARCOUNT
					NULL);

  const HParser *type = int_range(uint16(), 1, 16);

  const HParser *qtype = choice(type, 
				 int_range(uint16(), 252, 255),
				 NULL);

  const HParser *class = int_range(uint16(), 1, 4);
  
  const HParser *qclass = choice(class,
				  int_range(uint16(), 255, 255),
				  NULL);

  const HParser *dns_question = sequence(sequence(many1(length_value(uint8(), 
								      uint8())), 
						   ch('\x00'),
						   NULL),  // QNAME
					  qtype,           // QTYPE
					  qclass,          // QCLASS
					  NULL);
 

  const HParser *dns_rr = sequence(domain,                          // NAME
				    type,                            // TYPE
				    class,                           // CLASS
				    uint32(),                        // TTL
				    length_value(uint16(), uint8()), // RDLENGTH+RDATA
				    NULL);


  dns_message = (HParser*)attr_bool(sequence(dns_header,
					      many(dns_question),
					      many(dns_rr),
					      end_p(),
					      NULL),
				     validate_dns);

  return dns_message;
}

int main(int argc, char** argv) {
  return 0;
}
