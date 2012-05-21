#include "../hammer.h"

bool is_zero(parse_result_t *p) {

}

int main(int argc, char **argv) {

  const parser_t dns_header = sequence(bits(16), // ID
				       bits(1),  // QR
				       bits(4),  // opcode
				       bits(1),  // AA
				       bits(1),  // TC
				       bits(1),  // RD
				       bits(1),  // RA
				       ignore(attr_bool(bits(3), is_zero)), // Z
				       bits(4),  // RCODE
				       uint16(), // QDCOUNT
				       uint16(), // ANCOUNT
				       uint16(), // NSCOUNT
				       uint16(), // ARCOUNT
				       NULL);

  const parser_t *dns_question = sequence(;

  bool validate_dns(parse_result_t *p) {

  }

  const parser_t *dns_message = attr_bool(sequence(dns_header,
						   many(dns_question),
						   many(dns_answer),
						   many(dns_authority),
						   many(dns_additional),
						   NULL),
					  validate_dns);
