// pj_str.c
// Some memory tests around pj_str_t
//
// To compile,
//
// $ gcc -g -I$PJSIP/pjlib/include -L$PJSIP/pjlib/lib -lpj-i386-apple-darwin12.5.0 -o pj_str pj_str.c
//
// And then run ./pj_str
//
// Check memory leaks using valgrind:
//
// $ valgrind --leak-check=full ./pj_str

#include <pjlib.h>

static void my_perror(const char *title, pj_status_t status)
{
  char errmsg[PJ_ERR_MSG_SIZE];
  pj_strerror(status, errmsg, sizeof(errmsg));
  fprintf(stderr, "%s: %s [status=%d]", title, errmsg, status);
}

#define PRINT_PSTR_DETAILS(pstr) print_pstr_details(#pstr, pstr)

void print_pstr_details(char *exp, const pj_str_t *pstr) {
  fprintf(stderr, "%s: %p, %p, %ld, %.*s\n", exp, pstr, pstr->ptr, pstr->slen, (int)pstr->slen, pstr->ptr);
}

pj_str_t test1() {
  return pj_str("Hello");
}

pj_str_t test2() {
  char *str = "Hello";
  return pj_str(str);
}

pj_str_t test3() {
  char str[] = "Hello";
  return pj_str(str);
}

const pj_str_t *test4() {
  pj_str_t pstr = pj_str("Hello");
  return &pstr;
}

const pj_str_t *test5() {
  return pj_cstr(malloc(sizeof(pj_str_t)), "Hello");
}

const pj_str_t *test6(pj_pool_t *pool) {
  return pj_cstr(pj_pool_alloc(pool, sizeof(pj_str_t)), "Hello");
}

pj_str_t test7() {
  char original[] = "Hello";
  char *dup = malloc(sizeof(original));
  strncpy(dup, original, sizeof(original));
  return pj_str(dup);
}

pj_str_t test8(pj_pool_t *pool) {
  char original[] = "Hello";
  char *dup = pj_pool_alloc(pool, sizeof(original));
  strncpy(dup, original, sizeof(original));
  return pj_str(dup);
}

int main()
{
  pj_caching_pool cp;
  pj_status_t status;
  // Must init PJLIB before anything else
  status = pj_init();
  if (status != PJ_SUCCESS) {
    my_perror("Error initializing PJLIB", status);
    return 1;
  }
  // Create the pool factory, in this case, a caching pool,
  // using default pool policy.
  pj_caching_pool_init(&cp, NULL, 1024*1024 );

  pj_pool_t *pool = pj_pool_create(&cp.factory, "pool1", 4000, 4000, NULL);

  pj_str_t test1_str = test1();
  PRINT_PSTR_DETAILS(&test1_str);

  pj_str_t test2_str = test2();
  PRINT_PSTR_DETAILS(&test2_str);

  pj_str_t test3_str = test3();
  PRINT_PSTR_DETAILS(&test3_str);

  PRINT_PSTR_DETAILS(test4());

  PRINT_PSTR_DETAILS(test5());

  PRINT_PSTR_DETAILS(test6(pool));

  pj_str_t test7_str = test7();
  PRINT_PSTR_DETAILS(&test7_str);

  pj_str_t test8_str = test8(pool);
  PRINT_PSTR_DETAILS(&test8_str);

  pj_pool_release(pool);

  // Done with demos, destroy caching pool before exiting app.
  pj_caching_pool_destroy(&cp);
  return 0;
}
