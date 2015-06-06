/* See LICENSE file for license and copyright information */

#include <check.h>
#include <fiu.h>

Suite* suite_document(void);
Suite* suite_page(void);
Suite* suite_outline(void);
Suite* suite_attachments(void);
Suite* suite_metadata(void);
Suite* suite_search(void);
Suite* suite_select(void);
Suite* suite_links(void);
Suite* suite_render(void);
Suite* suite_images(void);
Suite* suite_form_fields(void);
Suite* suite_annotations(void);

int main(void)
{
  /* initialize libfiu */
#ifdef FIU_ENABLE
  fiu_init(0);
#endif

  /* setup test suite */
  SRunner* suite_runner = srunner_create(NULL);
  srunner_set_fork_status(suite_runner, CK_NOFORK);

  srunner_add_suite(suite_runner, suite_document());
  srunner_add_suite(suite_runner, suite_page());
  srunner_add_suite(suite_runner, suite_outline());
  srunner_add_suite(suite_runner, suite_attachments());
  srunner_add_suite(suite_runner, suite_metadata());
  srunner_add_suite(suite_runner, suite_search());
  srunner_add_suite(suite_runner, suite_select());
  srunner_add_suite(suite_runner, suite_links());
  srunner_add_suite(suite_runner, suite_render());
  srunner_add_suite(suite_runner, suite_images());
  srunner_add_suite(suite_runner, suite_form_fields());
  srunner_add_suite(suite_runner, suite_annotations());

  int number_failed = 0;
  srunner_run_all(suite_runner, CK_ENV);
  number_failed += srunner_ntests_failed(suite_runner);
  srunner_free(suite_runner);

  return (number_failed == 0) ? 0 : 1;
}
