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

static void setup_document_render(void) {
  setup_document_with_path(&plugin_manager, &document, "files/render.pdf");
  fail_unless(zathura_document_get_page(document, 0, &page) == ZATHURA_ERROR_OK);
  fail_unless(page != NULL);
}

static void teardown_document(void) {
  fail_unless(zathura_document_free(document) == ZATHURA_ERROR_OK);
  document = NULL;

  fail_unless(zathura_plugin_manager_free(plugin_manager) == ZATHURA_ERROR_OK);
  plugin_manager = NULL;
}

START_TEST(test_pdf_page_render_invalid) {
#ifdef HAVE_CAIRO
  cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
  cairo_t* cairo = cairo_create(surface);

  fail_unless(zathura_page_render_cairo(NULL, NULL, 1.0, 0, 0)  == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(zathura_page_render_cairo(page, NULL, 1.0, 0, 0)  == ZATHURA_ERROR_INVALID_ARGUMENTS);
  fail_unless(zathura_page_render_cairo(NULL, cairo, 1.0, 0, 0) == ZATHURA_ERROR_INVALID_ARGUMENTS);

  cairo_destroy(cairo);
  cairo_surface_destroy(surface);
#endif
} END_TEST

static bool
compare_pixel(cairo_surface_t* surface, unsigned int x, unsigned int y, int
    rgb[3])
{
  const int rowstride  = cairo_image_surface_get_stride(surface);
  unsigned char* image = cairo_image_surface_get_data(surface);

  unsigned char* data = image + y * rowstride + x * 4;

  fail_unless(data[0] == rgb[0]);
  fail_unless(data[1] == rgb[1]);
  fail_unless(data[2] == rgb[2]);

  return true;
}

START_TEST(test_pdf_page_render) {
  unsigned int page_width;
  fail_unless(zathura_page_get_width(page, &page_width) == ZATHURA_ERROR_OK);
  unsigned int page_height;
  fail_unless(zathura_page_get_height(page, &page_height) == ZATHURA_ERROR_OK);

#ifdef HAVE_CAIRO
  cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
      page_width, page_height);
  cairo_t* cairo = cairo_create(surface);
  cairo_save(cairo);

  fail_unless(zathura_page_render_cairo(page, cairo, 1.0, 0, 0) == ZATHURA_ERROR_OK);
  cairo_restore(cairo);

  cairo_set_operator(cairo, CAIRO_OPERATOR_DEST_OVER);
  cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
  cairo_paint(cairo);

  fail_unless(compare_pixel(surface, 0, 0, (int[3]) {0,0,0})       == true);
  fail_unless(compare_pixel(surface, 0, 1, (int[3]) {255,255,255}) == true);
  fail_unless(compare_pixel(surface, 1, 0, (int[3]) {255,255,255}) == true);
  fail_unless(compare_pixel(surface, 1, 1, (int[3]) {0,0,0})       == true);

  cairo_destroy(cairo);
  cairo_surface_destroy(surface);
#endif
} END_TEST

Suite*
suite_render(void)
{
  TCase* tcase = NULL;
  Suite* suite = suite_create("render");

  tcase = tcase_create("basic");
  tcase_add_checked_fixture(tcase, setup_document_render, teardown_document);
  tcase_add_test(tcase, test_pdf_page_render_invalid);
  tcase_add_test(tcase, test_pdf_page_render);
  suite_add_tcase(suite, tcase);

  return suite;
}
