#include "../src/hammer.h"
#include "dns_common.h"

#define false 0
#define true 1

/**
 * A label can't be more than 63 characters.
 */
bool validate_label(parse_result_t *p) {
  if (TT_SEQUENCE != p->ast->token_type)
    return false;
  return (64 > p->ast->seq->used);
}

const parser_t* init_domain() {
  static const parser_t *domain = NULL;
  if (domain)
    return domain;

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

  const HParser *subdomain = sepBy1(label, ch('.'));

  domain = choice(subdomain, 
		  ch(' '), 
		  NULL); 

  return domain;
}

const HParser* init_character_string() {
  static const HParser *cstr = NULL;
  if (cstr)
    return cstr;

  cstr = length_value(uint8(), uint8());

  return cstr;
}
