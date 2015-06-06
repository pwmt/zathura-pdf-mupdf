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
#include <libzathura/libzathura.h>

#include "plugin.h"
#include "utils.h"

zathura_document_t* document;
zathura_plugin_manager_t* plugin_manager;
zathura_page_t* page;

static void setup_document_select(void) {
  setup_document_with_path(&plugin_manager, &document, "files/select.pdf");
  fail_unless(zathura_document_get_page(document, 0, &page) == ZATHURA_ERROR_OK);
}

static void teardown_document(void) {
  fail_unless(zathura_document_free(document) == ZATHURA_ERROR_OK);
  document = NULL;

  fail_unless(zathura_plugin_manager_free(plugin_manager) == ZATHURA_ERROR_OK);
  plugin_manager = NULL;
}

START_TEST(test_pdf_page_get_text_invalid) {
  fail_unless(pdf_page_get_text(NULL, NULL) == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(pdf_page_get_text(page, NULL) == ZATHURA_ERROR_INVALID_ARGUMENTS);
} END_TEST

START_TEST(test_pdf_page_get_text_valid) {
  char* text;
  fail_unless(pdf_page_get_text(page, &text) == ZATHURA_ERROR_OK);
  fail_unless(strcmp(text, "word") == 0);
} END_TEST

START_TEST(test_pdf_page_get_selected_text_invalid) {
  zathura_rectangle_t rectangle;

  fail_unless(pdf_page_get_selected_text(NULL, NULL, rectangle) == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(pdf_page_get_selected_text(page, NULL, rectangle) == ZATHURA_ERROR_INVALID_ARGUMENTS);
} END_TEST

START_TEST(test_pdf_page_get_selected_text_valid) {
  unsigned int page_height;
  unsigned int page_width;
  fail_unless(zathura_page_get_height(page, &page_height) == ZATHURA_ERROR_OK);
  fail_unless(zathura_page_get_width(page, &page_width) == ZATHURA_ERROR_OK);

  char* text;
  zathura_rectangle_t rectangle = { {0,0}, {page_width,page_height}};

  fail_unless(pdf_page_get_selected_text(page, &text, rectangle) == ZATHURA_ERROR_OK);
  fail_unless(strcmp(text, "word") == 0);

  zathura_rectangle_t rectangle2 = { {0,0}, {0,0}};

  fail_unless(pdf_page_get_selected_text(page, &text, rectangle2) == ZATHURA_ERROR_OK);
  fail_unless(strcmp(text, "") == 0);
} END_TEST

Suite*
suite_select(void)
{
  TCase* tcase = NULL;
  Suite* suite = suite_create("select");

  tcase = tcase_create("basic");
  tcase_add_checked_fixture(tcase, setup_document_select, teardown_document);
  tcase_add_test(tcase, test_pdf_page_get_text_invalid);
  tcase_add_test(tcase, test_pdf_page_get_text_valid);
  tcase_add_test(tcase, test_pdf_page_get_selected_text_invalid);
  tcase_add_test(tcase, test_pdf_page_get_selected_text_valid);
  suite_add_tcase(suite, tcase);

  return suite;
}
