#include "../src/hammer.h"
#include "dns_common.h"

#define false 0
#define true 1

bool is_zero(parse_result_t *p) {
  if (TT_UINT != p->ast->token_type)
    return false;
  return (0 == p->ast->uint);
}

/**
 * Every DNS message should have QDCOUNT entries in the question
 * section, and ANCOUNT+NSCOUNT+ARCOUNT resource records.
 */
bool validate_dns(parse_result_t *p) {
  if (TT_SEQUENCE != p->ast->token_type)
    return false;
  // The header holds the counts as its last 4 elements.
  parsed_token_t **elems = p->ast->seq->elements[0]->seq->elements;
  size_t qd = elems[8]->uint;
  size_t an = elems[9]->uint;
  size_t ns = elems[10]->uint;
  size_t ar = elems[11]->uint;
  parsed_token_t *questions = p->ast->seq->elements[1];
  if (questions->seq->used != qd)
    return false;
  parsed_token_t *rrs = p->ast->seq->elements[2];
  if (an+ns+ar != rrs->seq->used)
    return false;
  return true;
}

const parser_t* init_parser() {
  static parser_t *dns_message = NULL;
  if (dns_message)
    return dns_message;

  const parser_t *domain = init_domain();

  const parser_t *dns_header = sequence(bits(16, false), // ID
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

  const parser_t *type = int_range(uint16(), 1, 16);

  const parser_t *qtype = choice(type, 
				 int_range(uint16(), 252, 255),
				 NULL);

  const parser_t *class = int_range(uint16(), 1, 4);
  
  const parser_t *qclass = choice(class,
				  int_range(uint16(), 255, 255),
				  NULL);

  const parser_t *dns_question = sequence(sequence(many1(length_value(uint8(), 
								      uint8())), 
						   ch('\x00'),
						   NULL),  // QNAME
					  qtype,           // QTYPE
					  qclass,          // QCLASS
					  NULL);
 

  const parser_t *dns_rr = sequence(domain,                          // NAME
				    type,                            // TYPE
				    class,                           // CLASS
				    uint32(),                        // TTL
				    length_value(uint16(), uint8()), // RDLENGTH+RDATA
				    NULL);


  dns_message = (parser_t*)attr_bool(sequence(dns_header,
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
