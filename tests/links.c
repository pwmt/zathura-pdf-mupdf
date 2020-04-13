/* See LICENSE file for license and copyright information */

#include <check.h>
#include <fiu.h>
#include <fiu-control.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib/gstdio.h>

#include <libzathura/plugin-manager.h>
#include <libzathura/plugin-api.h>
#include <libzathura/libzathura.h>

#include <zathura-pdf-mupdf/plugin.h>
#include "utils.h"

zathura_document_t* document;
zathura_plugin_manager_t* plugin_manager;
zathura_page_t* page;

static void setup_document_links(void) {
  setup_document_with_path(&plugin_manager, &document, "files/links.pdf");
  fail_unless(zathura_document_get_page(document, 0, &page) == ZATHURA_ERROR_OK);
  fail_unless(page != NULL);
}

static void teardown_document(void) {
  fail_unless(zathura_document_free(document) == ZATHURA_ERROR_OK);
  document = NULL;

  fail_unless(zathura_plugin_manager_free(plugin_manager) == ZATHURA_ERROR_OK);
  plugin_manager = NULL;
}

START_TEST(test_pdf_page_get_links_invalid) {
  zathura_list_t* links;
  fail_unless(zathura_page_get_links(NULL, NULL) == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(zathura_page_get_links(page, NULL) == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(zathura_page_get_links(NULL, &links) == ZATHURA_ERROR_INVALID_ARGUMENTS);
} END_TEST

START_TEST(test_pdf_page_get_links_simple) {
  zathura_list_t* links = NULL;
  fail_unless(zathura_page_get_links(page, &links) == ZATHURA_ERROR_OK);
  fail_unless(links != NULL);

  unsigned int number_of_links = zathura_list_length(links);
  fail_unless(number_of_links == 2);
  zathura_list_free_full(links, free);
} END_TEST

Suite*
create_suite(void)
{
  TCase* tcase = NULL;
  Suite* suite = suite_create("links");

  tcase = tcase_create("basic");
  tcase_add_checked_fixture(tcase, setup_document_links, teardown_document);
  tcase_add_test(tcase, test_pdf_page_get_links_invalid);
  tcase_add_test(tcase, test_pdf_page_get_links_simple);
  suite_add_tcase(suite, tcase);

  return suite;
}
