#include "../src/hammer.h"
#include "dns_common.h"

#define false 0
#define true 1

/**
 * A label can't be more than 63 characters.
 */
bool validate_label(HParseResult *p) {
  if (TT_SEQUENCE != p->ast->token_type)
    return false;
  return (64 > p->ast->seq->used);
}

const HParser* init_domain() {
  static const HParser *domain = NULL;
  if (domain)
    return domain;

  const HParser *letter = h_choice(h_ch_range('a', 'z'),
				   h_ch_range('A', 'Z'),
				   NULL);

  const HParser *let_dig = h_choice(letter,
				    h_ch_range('0', '9'),
				    NULL);

  const HParser *ldh_str = h_many1(h_choice(let_dig,
					    h_ch('-'),
					    NULL));

  const HParser *label = h_attr_bool(h_sequence(letter,
						h_optional(h_sequence(h_optional(ldh_str),
								      let_dig,
								      NULL)),
						NULL),
				     validate_label);

  /**
   * You could write it like this ...
   *   HParser *indirect_subdomain = h_indirect();
   *   const HParser *subdomain = h_choice(label,
   *				           h_sequence(indirect_subdomain,
   *					              h_ch('.'),
   *					              label,
   *					              NULL),
   *				           NULL);
   *   h_bind_indirect(indirect_subdomain, subdomain);
   *
   * ... but this is easier and equivalent
   */

  const HParser *subdomain = h_sepBy1(label, h_ch('.'));

  domain = h_choice(subdomain, 
		    h_ch(' '), 
		    NULL); 

  return domain;
}

const HParser* init_character_string() {
  static const HParser *cstr = NULL;
  if (cstr)
    return cstr;

  cstr = h_length_value(h_uint8(), h_uint8());

  return cstr;
}
