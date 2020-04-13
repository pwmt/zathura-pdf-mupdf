/* See LICENSE file for license and copyright information */

#include <check.h>
#include <fiu.h>

extern Suite* create_suite(void);

int main(int argc, char* argv[])
{
  /* initialize libfiu */
#ifdef FIU_ENABLE
  fiu_init(0);
#endif

  /* setup test suite */
  SRunner* suite_runner = srunner_create(create_suite());
  srunner_set_fork_status(suite_runner, CK_NOFORK);

  /* Set output file */
  if (argc == 2) {
    srunner_set_xml(suite_runner, argv[1]);
  }

  srunner_run_all(suite_runner, CK_ENV);
  const int number_failed = srunner_ntests_failed(suite_runner);

  /* Clean-up */
  srunner_free(suite_runner);

  return (number_failed == 0) ? 0 : 1;
}
