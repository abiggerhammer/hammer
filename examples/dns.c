#include "../hammer.h"

bool is_zero(parse_result_t *p) {
  return (0 == p->ast->uint);
}

bool validate_dns(parse_result_t *p) {
  
}

int main(int argc, char **argv) {

  const parser_t dns_header = sequence(bits(16, false), // ID
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

  const parser_t *dns_question = sequence(length_value(uint8(), uint8()), // QNAME
					  uint16(),                       // QTYPE
					  uint16(),                       // QCLASS
					  NULL);

  const parser_t *letter = choice(range('a', 'z'),
				  range('A', 'Z'),
				  NULL);

  const parser_t *let_dig = choice(letter,
				   range('0', '9'),
				   NULL);

  const parser_t *ldh_str = many1(choice(let_dig,
					 ch('-'),
					 NULL));

  const parser_t *label = sequence(letter,
				   optional(sequence(optional(ldh_str),
						     let_dig,
						     NULL)),
				   NULL);

  /**
   * You could write it like this ...
   *   parser_t *indirect_subdomain = indirect();
   *   const parser_t *subdomain = choice(label,
   *				          sequence(indirect_subdomain,
   *					           ch('.'),
   *					           label,
   *					           NULL),
   *				          NULL);
   *   bind_indirect(indirect_subdomain, subdomain);
   *
   * ... but this is easier and equivalent
   */

  parser_t *subdomain = sepBy1(label, ch('.'));

  const parser_t *domain = choice(subdomain, 
				  ch(' '), 
				  NULL);

  const parser_t *dns_rr = sequence(domain,                         // NAME
				    uint16(),                       // TYPE
				    uint16(),                       // CLASS
				    uint32(),                       // TTL
				    length_value(uint16(), uint8()) // RDLENGTH+RDATA
				    NULL);


  const parser_t *dns_message = attr_bool(sequence(dns_header,
						   dns_question,
						   many(dns_rr),
						   end_p(),
						   NULL),
					  validate_dns);
}
