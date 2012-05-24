#include "../src/hammer.h"

#define false 0
#define true 1

bool is_zero(parse_result_t *p) {
  if (TT_UINT != p->ast->token_type)
    return 0;
  return (0 == p->ast->uint);
}

/**
 * A label can't be more than 63 characters.
 */
bool validate_label(parse_result_t *p) {
  if (TT_SEQUENCE != p->ast->token_type)
    return 0;
  return (64 > p->ast->seq->used);
}

/**
 * Every DNS message should have QDCOUNT entries in the question
 * section, and ANCOUNT+NSCOUNT+ARCOUNT resource records.
 *
 */
bool validate_dns(parse_result_t *p) {
  if (TT_SEQUENCE != p->ast->token_type)
    return 0;
  // The header holds the counts as its last 4 elements.
  parsed_token_t *header = p->ast->seq->elements[0];
  size_t qd = ((parsed_token_t*)header->seq->elements[8])->uint;
  size_t an = ((parsed_token_t*)header->seq->elements[9])->uint;
  size_t ns = ((parsed_token_t*)header->seq->elements[10])->uint;
  size_t ar = ((parsed_token_t*)header->seq->elements[11])->uint;
  parsed_token_t *questions = p->ast->seq->elements[1];
  if (questions->seq->used != qd)
    return 0;
  parsed_token_t *rrs = p->ast->seq->elements[2];
  if (an+ns+ar != rrs->seq->used)
    return 0;
}

parser_t* init_parser() {
  static parser_t *dns_message = NULL;
  if (dns_message)
    return dns_message;

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

  const parser_t *dns_question = sequence(length_value(uint8(), uint8()), // QNAME
					  uint16(),                       // QTYPE
					  uint16(),                       // QCLASS
					  NULL);

  const parser_t *letter = choice(ch_range('a', 'z'),
				  ch_range('A', 'Z'),
				  NULL);

  const parser_t *let_dig = choice(letter,
				   ch_range('0', '9'),
				   NULL);

  const parser_t *ldh_str = many1(choice(let_dig,
					 ch('-'),
					 NULL));

  const parser_t *label = attr_bool(sequence(letter,
					     optional(sequence(optional(ldh_str),
							       let_dig,
							       NULL)),
					     NULL),
				    validate_label);

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

  const parser_t *subdomain = sepBy1(label, ch('.'));

  const parser_t *domain = choice(subdomain, 
				  ch(' '), 
				  NULL);

  const parser_t *dns_rr = sequence(domain,                         // NAME
				    uint16(),                       // TYPE
				    uint16(),                       // CLASS
				    uint32(),                       // TTL
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
