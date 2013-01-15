#include "../src/hammer.h"

enum DNSTokenType_ {
  TT_dns_message = TT_USER,
  TT_dns_header,
  TT_dns_label,
  TT_dns_qname,
  TT_dns_question,
  TT_dns_rr
};

typedef struct dns_header {
  uint16_t id;
  bool qr, aa, tc, rd, ra;
  char opcode, rcode;
  size_t question_count;
  size_t answer_count;
  size_t authority_count;
  size_t additional_count;
} dns_header_t;

typedef struct dns_label {
  size_t len;
  uint8_t *label;
} dns_label_t;

typedef struct dns_qname {
  size_t qlen;
  dns_label_t *labels;
} dns_qname_t;

typedef struct dns_question {
  dns_qname_t qname;
  uint16_t qtype;
  uint16_t qclass;
} dns_question_t;

typedef struct dns_rr {
  char* name;
  uint16_t type;
  uint16_t class;
  uint32_t ttl; // cmos is also acceptable.
  uint16_t rdlength;
  union {
    char* cname;
    struct {
      uint8_t* cpu;
      uint8_t* os;
    } hinfo;
    char* mb;
    char* md;
    char* mf;
    char* mg;
    struct {
      char* rmailbx;
      char* emailbx;
    } minfo;
    char* mr;
    struct {
      uint16_t preference;
      char* exchange;
    } mx;
    uint8_t* null;
    char* ns;
    char* ptr;
    struct {
      char* mname;
      char* rname;
      uint32_t serial;
      uint32_t refresh;
      uint32_t retry;
      uint32_t expire;
      uint32_t minimum;
    } soa;
    struct {
      size_t count;
      uint8_t** txt_data;
    } txt;
    uint32_t a;
    struct {
      uint32_t address;
      uint8_t protocol;
      size_t len;
      uint8_t* bit_map;
    } wks;
  };
} dns_rr_t;

typedef struct dns_message {
  dns_header_t header;
  dns_question_t *questions;
  dns_rr_t *answers;
  dns_rr_t *authority;
  dns_rr_t *additional;
} dns_message_t;
