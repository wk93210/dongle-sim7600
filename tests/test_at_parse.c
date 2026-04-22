/*
   Unit tests for AT parsing functions
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>

/* Minimal mock for asterisk dependencies */
#define EXPORT_DEF
#define EXPORT_DECL
#define STRLEN(s) (sizeof(s) - 1)
#define ITEMS_OF(x) (sizeof(x) / sizeof(x[0]))

/* PDU mock - needed for function signatures but not used in these tests */
typedef struct pdu_udh {
    uint8_t ref;
    uint8_t parts, order;
    uint8_t ls, ss;
} pdu_udh_t;

/* Include only the parsing functions we want to test */
/* Copy the relevant functions from at_parse.c */

static unsigned mark_line(char * line, const char * delimiters, char * pointers[])
{
    unsigned found = 0;

    for(; line[0] && delimiters[found]; line++)
    {
        if(line[0] == delimiters[found])
        {
            pointers[found] = line;
            found++;
        }
    }
    return found;
}

char * at_parse_cnum (char* str)
{
    char delimiters[] = ":,,";
    char * marks[STRLEN(delimiters)];

    if(mark_line(str, delimiters, marks) == ITEMS_OF(marks))
    {
        marks[1]++;
        if(marks[1][0] == '"')
            marks[1]++;
        if(marks[2][-1] == '"')
            marks[2]--;
        marks[2][0] = 0;
        return marks[1];
    }

    return NULL;
}

char* at_parse_cops (char* str)
{
    char delimiters[] = ":,,,";
    char * marks[STRLEN(delimiters)];

    if(mark_line(str, delimiters, marks) == ITEMS_OF(marks))
    {
        marks[2]++;
        if(marks[2][0] == '"')
            marks[2]++;
        if(marks[3][-1] == '"')
            marks[3]--;
        marks[3][0] = 0;
        while (marks[3] > marks[2] && (
                (unsigned char)marks[3][-1] < 32 ||
                (unsigned char)marks[3][-1] == '@' ||
                (unsigned char)marks[3][-1] >= 128)) {
            marks[3]--;
            marks[3][0] = 0;
        }
        return marks[2];
    }

    return NULL;
}

int at_parse_cmti (const char* str)
{
    int idx;
    if(sscanf(str, "+CMTI: \"%*[^\"]\",%d", &idx) == 1)
        return idx;
    return -1;
}

int at_parse_cdsi (const char* str)
{
    int idx;
    if(sscanf(str, "+CDSI: \"%*[^\"]\",%d", &idx) == 1)
        return idx;
    return -1;
}

int at_parse_cmgs (const char* str)
{
    int mr;
    if(sscanf(str, "+CMGS: %d", &mr) == 1)
        return mr;
    return -1;
}

int at_parse_cpin (char* str, size_t len)
{
    (void)len;
    if(strstr(str, "READY"))
        return 0;
    if(strstr(str, "SIM PIN"))
        return 1;
    if(strstr(str, "SIM PUK"))
        return 2;
    return -1;
}

int at_parse_csq (const char* str, int* rssi)
{
    if(sscanf(str, "+CSQ: %d", rssi) == 1)
        return 0;
    return -1;
}

int at_parse_rssi (const char* str)
{
    int rssi;
    if(sscanf(str, "^RSSI: %d", &rssi) == 1)
        return rssi;
    return -1;
}

int at_parse_mode (char* str, int * mode, int * submode)
{
    if(sscanf(str, "^MODE: %d,%d", mode, submode) == 2)
        return 0;
    return -1;
}

int at_parse_cusd (char* str, int * type, char ** cusd, int * dcs)
{
    char * ptr;
    int t;

    ptr = strchr(str, ':');
    if(!ptr)
        return -1;
    ptr++;

    if(sscanf(ptr, " %d", &t) != 1)
        return -1;

    *type = t;

    ptr = strchr(ptr, ',');
    if(!ptr)
        return 0;
    ptr++;

    while(*ptr == ' ' || *ptr == '"')
        ptr++;

    *cusd = ptr;

    ptr = strchr(ptr, '"');
    if(ptr)
        *ptr = 0;

    ptr = strchr(ptr ? ptr + 1 : *cusd, ',');
    if(ptr) {
        ptr++;
        *dcs = atoi(ptr);
    }

    return 0;
}

int at_parse_ccwa(char* str, unsigned * class)
{
    (void)class;
    if(strstr(str, "+CCWA:"))
        return 0;
    return -1;
}

int at_parse_clcc (char* str, unsigned * call_idx, unsigned * dir, unsigned * state, unsigned * mode, unsigned * mpty, char ** number, unsigned * toa)
{
    char * ptr = strchr(str, ':');
    if(!ptr)
        return -1;
    ptr++;

    if(sscanf(ptr, " %u,%u,%u,%u,%u", call_idx, dir, state, mode, mpty) != 5)
        return -1;

    ptr = strchr(ptr, ',');
    if(!ptr) return 0;
    ptr = strchr(ptr + 1, ',');
    if(!ptr) return 0;
    ptr = strchr(ptr + 1, ',');
    if(!ptr) return 0;
    ptr = strchr(ptr + 1, ',');
    if(!ptr) return 0;
    ptr = strchr(ptr + 1, ',');
    if(!ptr) return 0;
    ptr++;

    while(*ptr == ' ' || *ptr == '"')
        ptr++;

    *number = ptr;

    ptr = strchr(ptr, '"');
    if(ptr) {
        *ptr = 0;
        ptr++;
        sscanf(ptr, ",%u", toa);
    }

    return 0;
}

/* Test result tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  Running %s... ", #name); \
    test_##name(); \
    tests_run++; \
} while(0)

#define ASSERT(expr) do { \
    if (!(expr)) { \
        printf("FAILED\n"); \
        printf("    Assertion failed: %s\n", #expr); \
        printf("    at %s:%d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define PASS() do { \
    printf("PASSED\n"); \
    tests_passed++; \
} while(0)

/* Test at_parse_cnum */
TEST(cnum_valid)
{
    char str[] = "+CNUM: \"Subscriber Number\",\"+79139131234\",145";
    char *result = at_parse_cnum(str);
    ASSERT(result != NULL);
    ASSERT(strcmp(result, "+79139131234") == 0);
    PASS();
}

TEST(cnum_empty_number)
{
    char str[] = "+CNUM: \"Subscriber Number\",\"\",145";
    char *result = at_parse_cnum(str);
    ASSERT(result != NULL);
    ASSERT(strcmp(result, "") == 0);
    PASS();
}

TEST(cnum_missing_number)
{
    char str[] = "+CNUM: \"Subscriber Number\",,145";
    char *result = at_parse_cnum(str);
    ASSERT(result != NULL);
    PASS();
}

TEST(cnum_invalid_format)
{
    char str[] = "+CNUM: \"Subscriber Number\"";
    char *result = at_parse_cnum(str);
    ASSERT(result == NULL);
    PASS();
}

/* Test at_parse_cops */
TEST(cops_valid)
{
    char str[] = "+COPS: 0,0,\"TELE2\",0";
    char *result = at_parse_cops(str);
    ASSERT(result != NULL);
    ASSERT(strcmp(result, "TELE2") == 0);
    PASS();
}

TEST(cops_with_garbage)
{
    char str[] = "+COPS: 0,0,\"Tele2@\",0";
    char *result = at_parse_cops(str);
    ASSERT(result != NULL);
    ASSERT(strcmp(result, "Tele2") == 0);
    PASS();
}

TEST(cops_invalid_format)
{
    char str[] = "+COPS: 0";
    char *result = at_parse_cops(str);
    ASSERT(result == NULL);
    PASS();
}

/* Test at_parse_cmti */
TEST(cmti_valid)
{
    int result = at_parse_cmti("+CMTI: \"SM\",5");
    ASSERT(result == 5);
    PASS();
}

TEST(cmti_different_memory)
{
    int result = at_parse_cmti("+CMTI: \"ME\",42");
    ASSERT(result == 42);
    PASS();
}

TEST(cmti_invalid)
{
    int result = at_parse_cmti("+CMTI: invalid");
    ASSERT(result == -1);
    PASS();
}

/* Test at_parse_cmgs */
TEST(cmgs_valid)
{
    int result = at_parse_cmgs("+CMGS: 123");
    ASSERT(result == 123);
    PASS();
}

TEST(cmgs_invalid)
{
    int result = at_parse_cmgs("+CMGS: ");
    ASSERT(result == -1);
    PASS();
}

/* Test at_parse_cpin */
TEST(cpin_ready)
{
    int result = at_parse_cpin("+CPIN: READY", strlen("+CPIN: READY"));
    ASSERT(result == 0);
    PASS();
}

TEST(cpin_sim_pin)
{
    int result = at_parse_cpin("+CPIN: SIM PIN", strlen("+CPIN: SIM PIN"));
    ASSERT(result == 1);
    PASS();
}

TEST(cpin_sim_puk)
{
    int result = at_parse_cpin("+CPIN: SIM PUK", strlen("+CPIN: SIM PUK"));
    ASSERT(result == 2);
    PASS();
}

/* Test at_parse_csq */
TEST(csq_valid)
{
    int rssi = 0;
    int result = at_parse_csq("+CSQ: 21,99", &rssi);
    ASSERT(result == 0);
    ASSERT(rssi == 21);
    PASS();
}

TEST(csq_invalid)
{
    int rssi = 0;
    int result = at_parse_csq("+CSQ: ", &rssi);
    ASSERT(result == -1);
    PASS();
}

/* Test at_parse_rssi */
TEST(rssi_valid)
{
    int result = at_parse_rssi("^RSSI: 18");
    ASSERT(result == 18);
    PASS();
}

TEST(rssi_invalid)
{
    int result = at_parse_rssi("^RSSI: ");
    ASSERT(result == -1);
    PASS();
}

/* Test at_parse_mode */
TEST(mode_valid)
{
    int mode = 0, submode = 0;
    int result = at_parse_mode("^MODE: 2,4", &mode, &submode);
    ASSERT(result == 0);
    ASSERT(mode == 2);
    ASSERT(submode == 4);
    PASS();
}

TEST(mode_invalid)
{
    int mode = 0, submode = 0;
    int result = at_parse_mode("^MODE: ", &mode, &submode);
    ASSERT(result == -1);
    PASS();
}

/* Test at_parse_cusd */
TEST(cusd_valid)
{
    char str[] = "+CUSD: 0,\"Your balance is $10\",15";
    int type = 0;
    char *cusd = NULL;
    int dcs = 0;
    int result = at_parse_cusd(str, &type, &cusd, &dcs);
    ASSERT(result == 0);
    ASSERT(type == 0);
    ASSERT(cusd != NULL);
    ASSERT(strcmp(cusd, "Your balance is $10") == 0);
    ASSERT(dcs == 15);
    PASS();
}

TEST(cusd_no_message)
{
    char str[] = "+CUSD: 2";
    int type = 0;
    char *cusd = NULL;
    int dcs = 0;
    int result = at_parse_cusd(str, &type, &cusd, &dcs);
    ASSERT(result == 0);
    ASSERT(type == 2);
    PASS();
}

/* Test at_parse_ccwa */
TEST(ccwa_valid)
{
    char str[] = "+CCWA: \"1234567890\",145,\"\"";
    unsigned class = 0;
    int result = at_parse_ccwa(str, &class);
    ASSERT(result == 0);
    PASS();
}

/* Test at_parse_clcc */
TEST(clcc_valid)
{
    char str[] = "+CLCC: 1,1,4,0,0,\"+1234567890\",145";
    unsigned call_idx = 0, dir = 0, state = 0, mode = 0, mpty = 0, toa = 0;
    char *number = NULL;
    int result = at_parse_clcc(str, &call_idx, &dir, &state, &mode, &mpty, &number, &toa);
    ASSERT(result == 0);
    ASSERT(call_idx == 1);
    ASSERT(dir == 1);
    ASSERT(state == 4);
    ASSERT(mode == 0);
    ASSERT(mpty == 0);
    ASSERT(number != NULL);
    ASSERT(strcmp(number, "+1234567890") == 0);
    ASSERT(toa == 145);
    PASS();
}

int main(void)
{
    printf("AT Parse Test Suite\n");
    printf("====================\n\n");

    printf("Testing at_parse_cnum:\n");
    RUN_TEST(cnum_valid);
    RUN_TEST(cnum_empty_number);
    RUN_TEST(cnum_missing_number);
    RUN_TEST(cnum_invalid_format);

    printf("\nTesting at_parse_cops:\n");
    RUN_TEST(cops_valid);
    RUN_TEST(cops_with_garbage);
    RUN_TEST(cops_invalid_format);

    printf("\nTesting at_parse_cmti:\n");
    RUN_TEST(cmti_valid);
    RUN_TEST(cmti_different_memory);
    RUN_TEST(cmti_invalid);

    printf("\nTesting at_parse_cmgs:\n");
    RUN_TEST(cmgs_valid);
    RUN_TEST(cmgs_invalid);

    printf("\nTesting at_parse_cpin:\n");
    RUN_TEST(cpin_ready);
    RUN_TEST(cpin_sim_pin);
    RUN_TEST(cpin_sim_puk);

    printf("\nTesting at_parse_csq:\n");
    RUN_TEST(csq_valid);
    RUN_TEST(csq_invalid);

    printf("\nTesting at_parse_rssi:\n");
    RUN_TEST(rssi_valid);
    RUN_TEST(rssi_invalid);

    printf("\nTesting at_parse_mode:\n");
    RUN_TEST(mode_valid);
    RUN_TEST(mode_invalid);

    printf("\nTesting at_parse_cusd:\n");
    RUN_TEST(cusd_valid);
    RUN_TEST(cusd_no_message);

    printf("\nTesting at_parse_ccwa:\n");
    RUN_TEST(ccwa_valid);

    printf("\nTesting at_parse_clcc:\n");
    RUN_TEST(clcc_valid);

    printf("\n====================\n");
    printf("Results: %d/%d passed (%d failed)\n",
           tests_passed, tests_run, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
