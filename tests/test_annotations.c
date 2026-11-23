/* SPDX-License-Identifier: Zlib */

#include <glib/gstdio.h>
#include <cairo.h>

#include "plugin.h"
#include "zathura-stubs.h"

#define PAGE_WIDTH 612
#define PAGE_HEIGHT 792

typedef struct {
  char* dir;
  char* file;
} fixture_t;

typedef struct {
  zathura_document_t* document;
  zathura_page_t* page;
} opened_t;

/* a one page PDF with two lines of text */
static char* make_pdf(void) {
  const char* content     = "BT /F1 24 Tf 72 700 Td (Hello annotation world) Tj 0 -40 Td (Second line of text) Tj ET";
  g_autofree char* stream = g_strdup_printf("<< /Length %zu >>\nstream\n%s\nendstream", strlen(content), content);
  const char* objects[]   = {
      "<< /Type /Catalog /Pages 2 0 R >>",
      "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
      "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >>",
      stream,
      "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
  };
  const size_t n = G_N_ELEMENTS(objects);

  GString* pdf = g_string_new("%PDF-1.7\n");
  size_t offsets[G_N_ELEMENTS(objects)];
  for (size_t i = 0; i < n; ++i) {
    offsets[i] = pdf->len;
    g_string_append_printf(pdf, "%zu 0 obj\n%s\nendobj\n", i + 1, objects[i]);
  }
  const size_t xref = pdf->len;
  g_string_append_printf(pdf, "xref\n0 %zu\n0000000000 65535 f \n", n + 1);
  for (size_t i = 0; i < n; ++i) {
    g_string_append_printf(pdf, "%010zu 00000 n \n", offsets[i]);
  }
  g_string_append_printf(pdf, "trailer\n<< /Size %zu /Root 1 0 R >>\nstartxref\n%zu\n%%%%EOF\n", n + 1, xref);
  return g_string_free(pdf, FALSE);
}

static void fixture_set_up(fixture_t* fixture, gconstpointer UNUSED_data) {
  (void)UNUSED_data;
  g_autoptr(GError) error = NULL;
  fixture->dir            = g_dir_make_tmp("zathura-pdf-mupdf-XXXXXX", &error);
  g_assert_no_error(error);
  fixture->file        = g_build_filename(fixture->dir, "document.pdf", NULL);
  g_autofree char* pdf = make_pdf();
  g_assert_true(g_file_set_contents(fixture->file, pdf, -1, &error));
}

static void fixture_tear_down(fixture_t* fixture, gconstpointer UNUSED_data) {
  (void)UNUSED_data;
  g_autoptr(GDir) dir = g_dir_open(fixture->dir, 0, NULL);
  const char* name    = NULL;
  while (dir != NULL && (name = g_dir_read_name(dir)) != NULL) {
    g_autofree char* path = g_build_filename(fixture->dir, name, NULL);
    g_remove(path);
  }
  g_rmdir(fixture->dir);
  g_free(fixture->file);
  g_free(fixture->dir);
}

static opened_t open_document(const char* path) {
  opened_t opened = {.document = stub_document_new(path)};
  g_assert_cmpint(pdf_document_open(opened.document), ==, ZATHURA_ERROR_OK);
  opened.page = opened.document->pages[0];
  g_assert_cmpint(pdf_page_init(opened.page), ==, ZATHURA_ERROR_OK);
  return opened;
}

static void close_document(opened_t* opened) {
  pdf_page_clear(opened->page, opened->page->data);
  pdf_document_free(opened->document, opened->document->data);
  stub_document_free(opened->document);
}

static girara_list_t* annotations(opened_t* opened) {
  zathura_error_t error = ZATHURA_ERROR_OK;
  girara_list_t* list   = pdf_page_annotations_get(opened->page, opened->page->data, &error);
  g_assert_cmpint(error, ==, ZATHURA_ERROR_OK);
  g_assert_nonnull(list);
  return list;
}

/* pixels that are yellow, which is the color of the highlights below */
static int count_yellow(opened_t* opened) {
  cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PAGE_WIDTH, PAGE_HEIGHT);
  cairo_t* cairo           = cairo_create(surface);
  g_assert_cmpint(pdf_page_render_cairo(opened->page, opened->page->data, cairo, false), ==, ZATHURA_ERROR_OK);
  cairo_surface_flush(surface);

  int yellow                = 0;
  const unsigned char* data = cairo_image_surface_get_data(surface);
  const int stride          = cairo_image_surface_get_stride(surface);
  for (int y = 0; y < PAGE_HEIGHT; ++y) {
    for (int x = 0; x < PAGE_WIDTH; ++x) {
      const unsigned char* pixel = data + y * stride + x * 4; /* BGRA */
      if (pixel[2] > 200 && pixel[1] > 200 && pixel[0] < 120) {
        ++yellow;
      }
    }
  }
  cairo_destroy(cairo);
  cairo_surface_destroy(surface);
  return yellow;
}

/* the box around the first characters of the page */
static zathura_rectangle_t first_word_box(opened_t* opened, size_t n_chars) {
  zathura_text_layout_t layout = {0};
  g_assert_cmpint(pdf_page_get_text_layout(opened->page, opened->page->data, &layout), ==, ZATHURA_ERROR_OK);
  g_assert_true(g_str_has_prefix(layout.text, "Hello annotation"));
  g_assert_cmpuint(layout.n_rects, >, n_chars);

  zathura_rectangle_t box = layout.rects[0];
  for (size_t i = 1; i < n_chars; ++i) {
    box.x1 = MIN(box.x1, layout.rects[i].x1);
    box.y1 = MIN(box.y1, layout.rects[i].y1);
    box.x2 = MAX(box.x2, layout.rects[i].x2);
    box.y2 = MAX(box.y2, layout.rects[i].y2);
  }
  g_free(layout.text);
  g_free(layout.rects);
  return box;
}

static void add_highlight(opened_t* opened, zathura_rectangle_t box, const char* contents) {
  zathura_annotation_t annotation = {
      .type      = ZATHURA_ANNOTATION_HIGHLIGHT,
      .quads     = &box,
      .n_quads   = 1,
      .rect      = box,
      .has_color = true,
      .color     = {1, 1, 0},
      .contents  = (char*)contents,
  };
  g_assert_cmpint(pdf_page_annotation_add(opened->page, opened->page->data, &annotation), ==, ZATHURA_ERROR_OK);
}

static void test_empty(fixture_t* fixture, gconstpointer UNUSED_data) {
  (void)UNUSED_data;
  opened_t opened               = open_document(fixture->file);
  g_autoptr(girara_list_t) list = annotations(&opened);
  g_assert_cmpuint(girara_list_size(list), ==, 0);
  g_assert_cmpint(count_yellow(&opened), ==, 0);
  close_document(&opened);
}

static void test_add_and_list(fixture_t* fixture, gconstpointer UNUSED_data) {
  (void)UNUSED_data;
  opened_t opened               = open_document(fixture->file);
  const zathura_rectangle_t box = first_word_box(&opened, 5);

  add_highlight(&opened, box, "my comment");
  zathura_annotation_t note = {.type      = ZATHURA_ANNOTATION_TEXT,
                               .rect      = {300, 100, 320, 120},
                               .has_color = true,
                               .color     = {1, 0, 0},
                               .contents  = "a note"};
  g_assert_cmpint(pdf_page_annotation_add(opened.page, opened.page->data, &note), ==, ZATHURA_ERROR_OK);

  g_autoptr(girara_list_t) list = annotations(&opened);
  g_assert_cmpuint(girara_list_size(list), ==, 2);

  const zathura_annotation_t* highlight = girara_list_nth(list, 0);
  g_assert_cmpint(highlight->type, ==, ZATHURA_ANNOTATION_HIGHLIGHT);
  g_assert_cmpuint(highlight->n_quads, ==, 1);
  g_assert_cmpfloat_with_epsilon(highlight->quads[0].x1, box.x1, 0.5);
  g_assert_cmpfloat_with_epsilon(highlight->quads[0].y2, box.y2, 0.5);
  g_assert_true(highlight->has_color);
  g_assert_cmpfloat_with_epsilon(highlight->color[0], 1, 0.01);
  g_assert_cmpfloat_with_epsilon(highlight->color[1], 1, 0.01);
  g_assert_cmpfloat_with_epsilon(highlight->color[2], 0, 0.01);
  g_assert_cmpstr(highlight->contents, ==, "my comment");

  const zathura_annotation_t* sticky = girara_list_nth(list, 1);
  g_assert_cmpint(sticky->type, ==, ZATHURA_ANNOTATION_TEXT);
  g_assert_cmpstr(sticky->contents, ==, "a note");

  close_document(&opened);
}

static void test_render(fixture_t* fixture, gconstpointer UNUSED_data) {
  (void)UNUSED_data;
  opened_t opened = open_document(fixture->file);
  g_assert_cmpint(count_yellow(&opened), ==, 0);
  add_highlight(&opened, first_word_box(&opened, 5), NULL);
  /* the highlight is drawn over the text without reopening the document */
  g_assert_cmpint(count_yellow(&opened), >, 200);
  close_document(&opened);
}

static void test_invalid_arguments(fixture_t* fixture, gconstpointer UNUSED_data) {
  (void)UNUSED_data;
  opened_t opened = open_document(fixture->file);

  zathura_error_t error = ZATHURA_ERROR_OK;
  g_assert_null(pdf_page_annotations_get(NULL, NULL, &error));
  g_assert_cmpint(error, ==, ZATHURA_ERROR_INVALID_ARGUMENTS);

  zathura_rectangle_t box       = {0, 0, 10, 10};
  zathura_annotation_t other    = {.type = ZATHURA_ANNOTATION_OTHER};
  zathura_annotation_t no_quads = {.type = ZATHURA_ANNOTATION_UNDERLINE};
  zathura_annotation_t valid    = {.type = ZATHURA_ANNOTATION_UNDERLINE, .quads = &box, .n_quads = 1};
  g_assert_cmpint(pdf_page_annotation_add(opened.page, opened.page->data, NULL), ==, ZATHURA_ERROR_INVALID_ARGUMENTS);
  g_assert_cmpint(pdf_page_annotation_add(opened.page, opened.page->data, &other), ==, ZATHURA_ERROR_NOT_IMPLEMENTED);
  g_assert_cmpint(pdf_page_annotation_add(opened.page, opened.page->data, &no_quads), ==,
                  ZATHURA_ERROR_INVALID_ARGUMENTS);
  g_assert_cmpint(pdf_page_annotation_add(NULL, NULL, &valid), ==, ZATHURA_ERROR_INVALID_ARGUMENTS);

  g_assert_cmpint(pdf_page_annotation_delete(opened.page, opened.page->data, 0), ==, ZATHURA_ERROR_INVALID_ARGUMENTS);
  g_assert_cmpint(pdf_page_annotation_set_contents(opened.page, opened.page->data, 0, "x"), ==,
                  ZATHURA_ERROR_INVALID_ARGUMENTS);
  g_assert_cmpint(pdf_page_annotation_set_contents(opened.page, opened.page->data, 0, NULL), ==,
                  ZATHURA_ERROR_INVALID_ARGUMENTS);

  g_autoptr(girara_list_t) list = annotations(&opened);
  g_assert_cmpuint(girara_list_size(list), ==, 0);
  close_document(&opened);
}

static void test_set_contents_and_delete(fixture_t* fixture, gconstpointer UNUSED_data) {
  (void)UNUSED_data;
  opened_t opened = open_document(fixture->file);
  add_highlight(&opened, first_word_box(&opened, 5), "one");
  add_highlight(&opened, first_word_box(&opened, 3), "two");

  g_assert_cmpint(pdf_page_annotation_set_contents(opened.page, opened.page->data, 1, "changed"), ==, ZATHURA_ERROR_OK);
  {
    g_autoptr(girara_list_t) list = annotations(&opened);
    g_assert_cmpuint(girara_list_size(list), ==, 2);
    g_assert_cmpstr(((zathura_annotation_t*)girara_list_nth(list, 0))->contents, ==, "one");
    g_assert_cmpstr(((zathura_annotation_t*)girara_list_nth(list, 1))->contents, ==, "changed");
  }

  g_assert_cmpint(pdf_page_annotation_delete(opened.page, opened.page->data, 0), ==, ZATHURA_ERROR_OK);
  {
    g_autoptr(girara_list_t) list = annotations(&opened);
    g_assert_cmpuint(girara_list_size(list), ==, 1);
    g_assert_cmpstr(((zathura_annotation_t*)girara_list_nth(list, 0))->contents, ==, "changed");
  }

  g_assert_cmpint(pdf_page_annotation_delete(opened.page, opened.page->data, 0), ==, ZATHURA_ERROR_OK);
  g_assert_cmpint(count_yellow(&opened), ==, 0);
  close_document(&opened);
}

static void test_save_to_new_file(fixture_t* fixture, gconstpointer UNUSED_data) {
  (void)UNUSED_data;
  g_autofree char* copy = g_build_filename(fixture->dir, "copy.pdf", NULL);

  opened_t opened = open_document(fixture->file);
  add_highlight(&opened, first_word_box(&opened, 5), "kept");
  g_assert_cmpint(pdf_document_save_as(opened.document, opened.document->data, copy), ==, ZATHURA_ERROR_OK);
  close_document(&opened);

  opened_t reopened             = open_document(copy);
  g_autoptr(girara_list_t) list = annotations(&reopened);
  g_assert_cmpuint(girara_list_size(list), ==, 1);
  g_assert_cmpstr(((zathura_annotation_t*)girara_list_nth(list, 0))->contents, ==, "kept");
  g_assert_cmpint(count_yellow(&reopened), >, 200);
  close_document(&reopened);

  /* the original is untouched */
  opened_t original             = open_document(fixture->file);
  g_autoptr(girara_list_t) none = annotations(&original);
  g_assert_cmpuint(girara_list_size(none), ==, 0);
  close_document(&original);
}

static void test_save_over_opened_file(fixture_t* fixture, gconstpointer UNUSED_data) {
  (void)UNUSED_data;
  g_autofree char* before = NULL;
  gsize before_length     = 0;
  g_assert_true(g_file_get_contents(fixture->file, &before, &before_length, NULL));

  opened_t opened = open_document(fixture->file);
  add_highlight(&opened, first_word_box(&opened, 5), "first");
  g_assert_cmpint(pdf_document_save_as(opened.document, opened.document->data, fixture->file), ==, ZATHURA_ERROR_OK);

  /* the changes are appended: the original bytes are still at the start of the file */
  {
    g_autofree char* after = NULL;
    gsize after_length     = 0;
    g_assert_true(g_file_get_contents(fixture->file, &after, &after_length, NULL));
    g_assert_cmpuint(after_length, >, before_length);
    g_assert_cmpint(memcmp(before, after, before_length), ==, 0);
  }

  /* Saving again after more changes works as well, and the document stays usable. Appending a second time from the
   * same document would write a broken file, so the file is replaced by a complete new one. */
  g_assert_cmpint(g_chmod(fixture->file, 0640), ==, 0);
  add_highlight(&opened, first_word_box(&opened, 3), "second");
  g_assert_cmpint(pdf_document_save_as(opened.document, opened.document->data, fixture->file), ==, ZATHURA_ERROR_OK);
  add_highlight(&opened, first_word_box(&opened, 2), "third");
  g_assert_cmpint(pdf_document_save_as(opened.document, opened.document->data, fixture->file), ==, ZATHURA_ERROR_OK);
  {
    g_autoptr(girara_list_t) list = annotations(&opened);
    g_assert_cmpuint(girara_list_size(list), ==, 3);
  }
  close_document(&opened);

  /* the replacement keeps the permissions and leaves no temporary file behind */
  GStatBuf file_stat;
  g_assert_cmpint(g_stat(fixture->file, &file_stat), ==, 0);
  g_assert_cmpuint(file_stat.st_mode & 07777, ==, 0640);
  {
    g_autoptr(GDir) dir = g_dir_open(fixture->dir, 0, NULL);
    unsigned int files  = 0;
    while (g_dir_read_name(dir) != NULL) {
      ++files;
    }
    g_assert_cmpuint(files, ==, 1);
  }

  opened_t reopened             = open_document(fixture->file);
  g_autoptr(girara_list_t) list = annotations(&reopened);
  g_assert_cmpuint(girara_list_size(list), ==, 3);
  g_assert_cmpstr(((zathura_annotation_t*)girara_list_nth(list, 0))->contents, ==, "first");
  g_assert_cmpstr(((zathura_annotation_t*)girara_list_nth(list, 1))->contents, ==, "second");
  g_assert_cmpstr(((zathura_annotation_t*)girara_list_nth(list, 2))->contents, ==, "third");
  close_document(&reopened);
}

static void test_not_a_pdf(fixture_t* fixture, gconstpointer UNUSED_data) {
  (void)UNUSED_data;
  g_autofree char* svg = g_build_filename(fixture->dir, "image.svg", NULL);
  g_assert_true(g_file_set_contents(svg,
                                    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\">"
                                    "<rect width=\"50\" height=\"50\" fill=\"red\"/></svg>",
                                    -1, NULL));

  opened_t opened = open_document(svg);
  /* documents that are not PDFs have no annotations, and cannot get any */
  g_autoptr(girara_list_t) list = annotations(&opened);
  g_assert_cmpuint(girara_list_size(list), ==, 0);

  zathura_rectangle_t box         = {0, 0, 10, 10};
  zathura_annotation_t annotation = {.type = ZATHURA_ANNOTATION_HIGHLIGHT, .quads = &box, .n_quads = 1};
  g_assert_cmpint(pdf_page_annotation_add(opened.page, opened.page->data, &annotation), ==,
                  ZATHURA_ERROR_NOT_IMPLEMENTED);
  g_assert_cmpint(pdf_page_annotation_delete(opened.page, opened.page->data, 0), ==, ZATHURA_ERROR_INVALID_ARGUMENTS);
  g_assert_cmpint(pdf_document_save_as(opened.document, opened.document->data, fixture->file), !=, ZATHURA_ERROR_OK);
  close_document(&opened);
}

int main(int argc, char* argv[]) {
  g_test_init(&argc, &argv, NULL);
#define ADD(name, func) g_test_add(name, fixture_t, NULL, fixture_set_up, func, fixture_tear_down)
  ADD("/annotations/empty", test_empty);
  ADD("/annotations/add_and_list", test_add_and_list);
  ADD("/annotations/render", test_render);
  ADD("/annotations/invalid_arguments", test_invalid_arguments);
  ADD("/annotations/set_contents_and_delete", test_set_contents_and_delete);
  ADD("/annotations/save_to_new_file", test_save_to_new_file);
  ADD("/annotations/save_over_opened_file", test_save_over_opened_file);
  ADD("/annotations/not_a_pdf", test_not_a_pdf);
  return g_test_run();
}
