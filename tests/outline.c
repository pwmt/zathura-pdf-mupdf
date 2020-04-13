/* See LICENSE file for license and copyright information */

#include <check.h>
#include <stdio.h>
#include <glib/gstdio.h>

#include <libzathura/plugin-manager.h>
#include <libzathura/plugin-api.h>
#include <libzathura/libzathura.h>

#include <zathura-pdf-mupdf/plugin.h>
#include "utils.h"

zathura_document_t* document;
zathura_plugin_manager_t* plugin_manager;

static void setup_document_empty(void) {
  setup_document_plugin(&plugin_manager, &document);
}

static void setup_document_outline(void) {
  setup_document_with_path(&plugin_manager, &document, "files/outline.pdf");
}

static void teardown_document(void) {
  fail_unless(zathura_document_free(document) == ZATHURA_ERROR_OK);
  document = NULL;

  fail_unless(zathura_plugin_manager_free(plugin_manager) == ZATHURA_ERROR_OK);
  plugin_manager = NULL;
}

START_TEST(test_pdf_document_get_outline_invalid) {
  fail_unless(pdf_document_get_outline(NULL, NULL)     == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(pdf_document_get_outline(document, NULL) == ZATHURA_ERROR_INVALID_ARGUMENTS);
} END_TEST

START_TEST(test_pdf_document_get_outline_does_not_exist) {
  zathura_node_t* outline;
  fail_unless(pdf_document_get_outline(document, &outline) == ZATHURA_ERROR_DOCUMENT_OUTLINE_DOES_NOT_EXIST);
} END_TEST

static void compare_outline_element(zathura_node_t* node, const char* title)
{
  fail_unless(node != NULL);
  zathura_outline_element_t* element = zathura_node_get_data(node);
  const char* tmp_title;
  fail_unless(zathura_outline_element_get_title(element, &tmp_title) == ZATHURA_ERROR_OK);
  fail_unless(strcmp(title, tmp_title) == 0);
}

START_TEST(test_pdf_document_get_outline_simple) {
  zathura_node_t* outline;
  fail_unless(pdf_document_get_outline(document, &outline) == ZATHURA_ERROR_OK);

  /* check number of root childs */
  unsigned int number_of_children =
    zathura_node_get_number_of_children(outline);
  fail_unless(number_of_children == 2);

  zathura_node_t* node = zathura_node_get_nth_child(outline, 0);
  compare_outline_element(node, "Heading 1");

  node = zathura_node_get_nth_child(outline, 1);
  compare_outline_element(node, "Heading 2");

  number_of_children = zathura_node_get_number_of_children(node);
  fail_unless(number_of_children == 1);
  node = zathura_node_get_nth_child(node, 0);
  compare_outline_element(node, "Heading 2.1");

  fail_unless(zathura_outline_free(outline) == ZATHURA_ERROR_OK);
} END_TEST

Suite*
create_suite(void)
{
  TCase* tcase = NULL;
  Suite* suite = suite_create("outline");

  tcase = tcase_create("basic");
  tcase_add_checked_fixture(tcase, setup_document_empty, teardown_document);
  tcase_add_test(tcase, test_pdf_document_get_outline_invalid);
  tcase_add_test(tcase, test_pdf_document_get_outline_does_not_exist);
  suite_add_tcase(suite, tcase);

  tcase = tcase_create("simple");
  tcase_add_checked_fixture(tcase, setup_document_outline, teardown_document);
  tcase_add_test(tcase, test_pdf_document_get_outline_simple);
  suite_add_tcase(suite, tcase);

  return suite;
}
