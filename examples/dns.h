struct dns_header {
  uint16_t id;
  boolean qr, aa, tc, rd, ra;
  char opcode, z, rcode;
  size_t question_count;
  size_t answer_count;
  size_t authority_count;
  size_t additional_count;
};
struct dns_question {
  char** qname; // change to whatever format you want; I'm assuming you'll keep the length-prefixed terms.
  uint16_t qtype;
  uint16_t qclass;
};
struct dns_rr {
  char* name;
  uint16_t type;
  uint16_t class;
  uint32_t ttl; // cmos is also acceptable.
  uint16_t rdlength;
  union {
    // various types of rdata-specific data here...
  };
};

typedef struct dns_message {
  struct dns_header header;
  struct dns_question *questions; // end all these with null, just to be sure.
  struct dns_rr *answers;
  struct dns_rr *authority;
  struct dns_rr *additional;
}
