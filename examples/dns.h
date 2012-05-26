typedef int bool;
struct dns_header {
  uint16_t id;
  bool qr, aa, tc, rd, ra;
  char opcode, rcode;
  size_t question_count;
  size_t answer_count;
  size_t authority_count;
  size_t additional_count;
};
struct dns_qname {
  size_t qlen;
  struct {
    size_t len;
    uint8_t *label;
  } *labels;
};
struct dns_question {
  struct dns_qname qname;
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
      uint8_t** bit_map;
    } wks;
  };
};

typedef struct dns_message {
  struct dns_header header;
  struct dns_question *questions;
  struct dns_rr *answers;
  struct dns_rr *authority;
  struct dns_rr *additional;
} dns_message_t;
