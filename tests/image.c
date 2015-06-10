/* See LICENSE file for license and copyright information */

#include <check.h>
#include <stdbool.h>
#include <fiu.h>
#include <fiu-control.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib/gstdio.h>

#include <libzathura/plugin-manager.h>
#include <libzathura/plugin-api.h>
#include <libzathura/libzathura.h>

#include "plugin.h"
#include "utils.h"

zathura_document_t* document;
zathura_plugin_manager_t* plugin_manager;
zathura_page_t* page;

static void setup_document_images(void) {
  setup_document_with_path(&plugin_manager, &document, "files/images.pdf");
  fail_unless(zathura_document_get_page(document, 0, &page) == ZATHURA_ERROR_OK);
  fail_unless(page != NULL);
}

static void teardown_document(void) {
  fail_unless(zathura_document_free(document) == ZATHURA_ERROR_OK);
  document = NULL;

  fail_unless(zathura_plugin_manager_free(plugin_manager) == ZATHURA_ERROR_OK);
  plugin_manager = NULL;
}

START_TEST(test_pdf_page_get_images_invalid) {
  zathura_list_t* images;
  fail_unless(zathura_page_get_images(NULL, NULL) == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(zathura_page_get_images(page, NULL) == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(zathura_page_get_images(NULL, &images) == ZATHURA_ERROR_INVALID_ARGUMENTS);
} END_TEST

START_TEST(test_pdf_page_get_images) {
  zathura_list_t* images;
  fail_unless(zathura_page_get_images(page, &images) == ZATHURA_ERROR_OK);
  fail_unless(images != NULL);
  fail_unless(zathura_list_length(images) == 1);

  zathura_image_t* image = zathura_list_nth_data(images, 0);
  fail_unless(image != NULL);

  zathura_rectangle_t position;
  fail_unless(zathura_image_get_position(image, &position) == ZATHURA_ERROR_OK);
  fail_unless((int) position.p1.x == (int) 305);
  fail_unless((int) position.p1.y == (int) 56);
  fail_unless((int) position.p2.x == (int) 306);
  fail_unless((int) position.p2.y == (int) 57);
} END_TEST

START_TEST(test_pdf_page_get_images_fault_injection) {
  zathura_list_t* images;
  fiu_enable("libc/mm/calloc", 1, NULL, 0);
  fail_unless(zathura_page_get_images(page, &images) == ZATHURA_ERROR_OUT_OF_MEMORY);
  fiu_disable("libc/mm/calloc");
} END_TEST

#if HAVE_CAIRO
START_TEST(test_pdf_page_get_image_cairo_buffer) {
  zathura_list_t* images;
  fail_unless(zathura_page_get_images(page, &images) == ZATHURA_ERROR_OK);
  fail_unless(images != NULL);
  fail_unless(zathura_list_length(images) == 1);

  zathura_image_t* image = zathura_list_nth_data(images, 0);
  fail_unless(image != NULL);

  cairo_surface_t* surface;
  fail_unless(zathura_image_get_cairo_surface(image, &surface) == ZATHURA_ERROR_OK);
  fail_unless(surface != NULL);

  cairo_surface_destroy(surface);
} END_TEST
#endif

Suite*
suite_images(void)
{
  TCase* tcase = NULL;
  Suite* suite = suite_create("images");

  tcase = tcase_create("basic");
  tcase_add_checked_fixture(tcase, setup_document_images, teardown_document);
  tcase_add_test(tcase, test_pdf_page_get_images_invalid);
  tcase_add_test(tcase, test_pdf_page_get_images);
  tcase_add_test(tcase, test_pdf_page_get_images_fault_injection);
#if HAVE_CAIRO
  tcase_add_test(tcase, test_pdf_page_get_image_cairo_buffer);
#endif
  suite_add_tcase(suite, tcase);

  return suite;
}
