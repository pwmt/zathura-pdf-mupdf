/* See LICENSE file for license and copyright information */

#include <check.h>
#include <fiu.h>
#include <fiu-control.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib/gstdio.h>

#include <libzathura/plugin-manager.h>
#include <libzathura/plugin-api.h>
#include <libzathura/libzathura.h>

#include "plugin.h"
#include "utils.h"

zathura_document_t* document;
zathura_plugin_manager_t* plugin_manager;
zathura_page_t* page;

static void setup_document_form_fields(void) {
  setup_document_with_path(&plugin_manager, &document, "files/form-fields.pdf");
  fail_unless(zathura_document_get_page(document, 0, &page) == ZATHURA_ERROR_OK);
  fail_unless(page != NULL);
}

static void teardown_document(void) {
  fail_unless(zathura_document_free(document) == ZATHURA_ERROR_OK);
  document = NULL;

  fail_unless(zathura_plugin_manager_free(plugin_manager) == ZATHURA_ERROR_OK);
  plugin_manager = NULL;
}

START_TEST(test_pdf_page_get_form_fields_invalid) {
  zathura_list_t* form_fields;
  fail_unless(zathura_page_get_form_fields(NULL, NULL) == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(zathura_page_get_form_fields(page, NULL) == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(zathura_page_get_form_fields(NULL, &form_fields) == ZATHURA_ERROR_INVALID_ARGUMENTS);
} END_TEST

START_TEST(test_pdf_page_get_form_fields_simple) {
  zathura_list_t* form_fields = NULL;
  fail_unless(zathura_page_get_form_fields(page, &form_fields) == ZATHURA_ERROR_OK);
  fail_unless(form_fields != NULL);

  unsigned int number_of_form_fields = zathura_list_length(form_fields);
  fail_unless(number_of_form_fields == 20);

  /* compare first form field */
  zathura_form_field_mapping_t* form_field_mapping = zathura_list_nth_data(form_fields, 2);
  fail_unless(form_field_mapping != NULL);
  zathura_form_field_t* form_field = form_field_mapping->form_field;
  fail_unless(form_field != NULL);

  zathura_form_field_type_t type;
  fail_unless(zathura_form_field_get_type(form_field, &type) == ZATHURA_ERROR_OK);
  fail_unless(type == ZATHURA_FORM_FIELD_BUTTON);

  zathura_form_field_button_type_t button_type;
  fail_unless(zathura_form_field_button_get_type(form_field, &button_type) == ZATHURA_ERROR_OK);
  fail_unless(button_type == ZATHURA_FORM_FIELD_BUTTON_TYPE_RADIO);

  bool state;
  fail_unless(zathura_form_field_button_get_state(form_field, &state) == ZATHURA_ERROR_OK);
  fail_unless(state == false);

  /* compare second form field */
  form_field_mapping = zathura_list_nth_data(form_fields, 1);
  fail_unless(form_field_mapping != NULL);
  form_field = form_field_mapping->form_field;
  fail_unless(form_field != NULL);

  fail_unless(zathura_form_field_get_type(form_field, &type) == ZATHURA_ERROR_OK);
  fail_unless(type == ZATHURA_FORM_FIELD_BUTTON);

  char* name;
  fail_unless(zathura_form_field_get_name(form_field, &name) == ZATHURA_ERROR_OK);

  fail_unless(zathura_form_field_button_get_type(form_field, &button_type) == ZATHURA_ERROR_OK);
  fail_unless(button_type == ZATHURA_FORM_FIELD_BUTTON_TYPE_CHECK);

  fail_unless(zathura_form_field_button_get_state(form_field, &state) == ZATHURA_ERROR_OK);
  fail_unless(state == true);

  /* compare third form field */
  form_field_mapping = zathura_list_nth_data(form_fields, 3);
  fail_unless(form_field_mapping != NULL);
  form_field = form_field_mapping->form_field;
  fail_unless(form_field != NULL);

  fail_unless(zathura_form_field_get_type(form_field, &type) == ZATHURA_ERROR_OK);
  fail_unless(type == ZATHURA_FORM_FIELD_CHOICE);

  /* clean-up */
  zathura_list_free_full(form_fields, free);
} END_TEST

START_TEST(test_pdf_page_save_form_field) {
  zathura_list_t* form_fields = NULL;
  fail_unless(zathura_page_get_form_fields(page, &form_fields) == ZATHURA_ERROR_OK);
  fail_unless(form_fields != NULL);

  /* Read and check second form field */
  zathura_form_field_mapping_t* form_field_mapping = zathura_list_nth_data(form_fields, 7);
  fail_unless(form_field_mapping != NULL);
  zathura_form_field_t* form_field = form_field_mapping->form_field;
  fail_unless(form_field != NULL);

  zathura_form_field_type_t type;
  fail_unless(zathura_form_field_get_type(form_field, &type) == ZATHURA_ERROR_OK);
  fail_unless(type == ZATHURA_FORM_FIELD_BUTTON);

  zathura_form_field_button_type_t button_type;
  fail_unless(zathura_form_field_button_get_type(form_field, &button_type) == ZATHURA_ERROR_OK);
  fail_unless(button_type == ZATHURA_FORM_FIELD_BUTTON_TYPE_CHECK);

  bool state;
  fail_unless(zathura_form_field_button_get_state(form_field, &state) == ZATHURA_ERROR_OK);
  fail_unless(state == false);

  /* Change state */
  fail_unless(zathura_form_field_button_set_state(form_field, true) == ZATHURA_ERROR_OK);

  fail_unless(zathura_form_field_button_get_state(form_field, &state) == ZATHURA_ERROR_OK);
  fail_unless(state == true);

  /* Save form field */
  fail_unless(zathura_form_field_save(form_field) == ZATHURA_ERROR_OK);

  /* Save document */
  char* path;
  gint file_handle;
  if ((file_handle = g_file_open_tmp(NULL, &path, NULL)) != -1) {
    fail_unless(pdf_document_save_as(document, path) == ZATHURA_ERROR_OK);
    close(file_handle);
  }
  fail_unless(file_handle != -1);

  /* clean-up */
  zathura_list_free_full(form_fields, free);
  fail_unless(zathura_document_free(document) == ZATHURA_ERROR_OK);

  /* open saved document */
  setup_document_with_path(&plugin_manager, &document, path);
  fail_unless(zathura_document_get_page(document, 0, &page) == ZATHURA_ERROR_OK);
  fail_unless(page != NULL);

  form_fields = NULL;
  fail_unless(zathura_page_get_form_fields(page, &form_fields) == ZATHURA_ERROR_OK);
  fail_unless(form_fields != NULL);

  /* Read and check second form field */
  form_field_mapping = zathura_list_nth_data(form_fields, 7);
  fail_unless(form_field_mapping != NULL);
  form_field = form_field_mapping->form_field;
  fail_unless(form_field != NULL);

  fail_unless(zathura_form_field_get_type(form_field, &type) == ZATHURA_ERROR_OK);
  fail_unless(type == ZATHURA_FORM_FIELD_BUTTON);

  fail_unless(zathura_form_field_button_get_type(form_field, &button_type) == ZATHURA_ERROR_OK);
  fail_unless(button_type == ZATHURA_FORM_FIELD_BUTTON_TYPE_CHECK);

  fail_unless(zathura_form_field_button_get_state(form_field, &state) == ZATHURA_ERROR_OK);
  fail_unless(state == true);

  zathura_list_free_full(form_fields, free);

  /* clean-up */
  g_remove(path);
  g_free(path);
} END_TEST

Suite*
suite_form_fields(void)
{
  TCase* tcase = NULL;
  Suite* suite = suite_create("forms");

  tcase = tcase_create("basic");
  tcase_add_checked_fixture(tcase, setup_document_form_fields, teardown_document);
  tcase_add_test(tcase, test_pdf_page_get_form_fields_invalid);
  tcase_add_test(tcase, test_pdf_page_get_form_fields_simple);
  suite_add_tcase(suite, tcase);

  tcase = tcase_create("save");
  tcase_add_checked_fixture(tcase, setup_document_form_fields, teardown_document);
  tcase_add_test(tcase, test_pdf_page_save_form_field);
  suite_add_tcase(suite, tcase);

  return suite;
}
