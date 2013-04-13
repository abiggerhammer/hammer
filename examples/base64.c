#include "../src/hammer.h"

const HParser* document = NULL;

void init_parser(void)
{
    // CORE
    const HParser *digit = h_ch_range(0x30, 0x39);
    const HParser *alpha = h_choice(h_ch_range(0x41, 0x5a), h_ch_range(0x61, 0x7a), NULL);

    // AUX.
    const HParser *plus = h_ch('+');
    const HParser *slash = h_ch('/');
    const HParser *equals = h_ch('=');

    const HParser *bsfdig = h_choice(alpha, digit, plus, slash, NULL);
    const HParser *bsfdig_4bit = h_choice(
        h_ch('A'), h_ch('E'), h_ch('I'), h_ch('M'), h_ch('Q'), h_ch('U'),
        h_ch('Y'), h_ch('c'), h_ch('g'), h_ch('k'), h_ch('o'), h_ch('s'),
        h_ch('w'), h_ch('0'), h_ch('4'), h_ch('8'), NULL);
    const HParser *bsfdig_2bit = h_choice(h_ch('A'), h_ch('Q'), h_ch('g'), h_ch('w'), NULL);
    const HParser *base64_2 = h_sequence(bsfdig, bsfdig, bsfdig_4bit, equals, NULL);
    const HParser *base64_1 = h_sequence(bsfdig, bsfdig_2bit, equals, equals, NULL);
    const HParser *base64 = h_choice(base64_2, base64_1, NULL);
        // why does this parse "A=="?!
        // why does this parse "aaA=" but not "aA=="?!

    document = base64;
}


#include <string.h>
#include <assert.h>
#define TRUE (1)
#define FALSE (0)

void assert_parse(int expected, char *data) {
    const HParseResult *result;

    size_t datasize = strlen(data);
    result = h_parse(document, (void*)data, datasize);
    if((result != NULL) != expected) {
        printf("Test failed: %s\n", data);
    }
}

void test() {
    assert_parse(TRUE, "");
    assert_parse(TRUE, "YQ==");
    assert_parse(TRUE, "YXU=");
    assert_parse(TRUE, "YXVy");
    assert_parse(TRUE, "QVVSIFNBUkFG");
    assert_parse(TRUE, "QVVSIEhFUlUgU0FSQUY=");
    assert_parse(FALSE, "A");
    assert_parse(FALSE, "A=");
    assert_parse(FALSE, "A==");
    assert_parse(FALSE, "AAA==");
}


#include <stdio.h>

int main(int argc, char **argv)
{
    uint8_t input[102400];
    size_t inputsize;
    const HParseResult *result;

    init_parser();

    test();

    inputsize = fread(input, 1, sizeof(input), stdin);
    fprintf(stderr, "inputsize=%lu\ninput=", inputsize);
    fwrite(input, 1, inputsize, stderr);
    result = h_parse(document, input, inputsize);

    if(result) {
        fprintf(stderr, "parsed=%lld bytes\n", result->bit_length/8);
        h_pprint(stdout, result->ast, 0, 0);
        return 0;
    } else {
        return 1;
    }
}
