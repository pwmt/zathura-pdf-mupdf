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
zathura_page_t* page;

static void setup_document_empty(void) {
  setup_document_with_path(&plugin_manager, &document, "files/annotations.pdf");
  fail_unless(zathura_document_get_page(document, 0, &page) == ZATHURA_ERROR_OK);
  fail_unless(page != NULL);
}

static void teardown_document(void) {
  fail_unless(zathura_document_free(document) == ZATHURA_ERROR_OK);
  document = NULL;

  fail_unless(zathura_plugin_manager_free(plugin_manager) == ZATHURA_ERROR_OK);
  plugin_manager = NULL;
}

START_TEST(test_pdf_page_get_annotations_invalid) {
  fail_unless(pdf_page_get_annotations(NULL, NULL)     == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(pdf_page_get_annotations(page, NULL) == ZATHURA_ERROR_INVALID_ARGUMENTS);
} END_TEST

START_TEST(test_pdf_page_get_annotations_simple) {
  zathura_list_t* annotations;
  fail_unless(pdf_page_get_annotations(page, &annotations) == ZATHURA_ERROR_OK);
  fail_unless(annotations != NULL);

  fail_unless(zathura_list_length(annotations) == 23);

  zathura_list_free_full(annotations, zathura_annotation_free);
} END_TEST

Suite*
create_suite(void)
{
  TCase* tcase = NULL;
  Suite* suite = suite_create("annotations");

  tcase = tcase_create("basic");
  tcase_add_checked_fixture(tcase, setup_document_empty, teardown_document);
  tcase_add_test(tcase, test_pdf_page_get_annotations_invalid);
  tcase_add_test(tcase, test_pdf_page_get_annotations_simple);
  suite_add_tcase(suite, tcase);

  return suite;
}
