/* See LICENSE file for license and copyright information */

#include <stdlib.h>
#include <girara/datastructures.h>

#include "pdf.h"

void
plugin_register(zathura_document_plugin_t* plugin)
{
  girara_list_append(plugin->content_types, g_content_type_from_mime_type("application/pdf"));
  plugin->open_function  = pdf_document_open;
}

bool
pdf_document_open(zathura_document_t* document)
{
  if (document == NULL) {
    goto error_ret;
  }

  document->functions.document_free             = pdf_document_free;
  document->functions.page_get                  = pdf_page_get;
  document->functions.page_search_text          = pdf_page_search_text;
  document->functions.page_links_get            = pdf_page_links_get;
  document->functions.page_form_fields_get      = pdf_page_form_fields_get;
  document->functions.page_render               = pdf_page_render;
#if HAVE_CAIRO
  document->functions.page_render_cairo         = pdf_page_render_cairo;
#endif
  document->functions.page_free                 = pdf_page_free;

  document->data = malloc(sizeof(pdf_document_t));
  if (document->data == NULL) {
    goto error_ret;
  }

  pdf_document_t* pdf_document = (pdf_document_t*) document->data;

  fz_accelerate();
  pdf_document->glyph_cache = fz_new_glyph_cache();

  if (pdf_open_xref(&(pdf_document->document), document->file_path, (char*) document->password) != fz_okay) {
    fprintf(stderr, "error: could not open file\n");
    goto error_free;
  }

  if (pdf_load_page_tree(pdf_document->document) != fz_okay) {
    fprintf(stderr, "error: could not open file\n");
    goto error_free;
  }

  document->number_of_pages = pdf_count_pages(pdf_document->document);

  return true;

error_free:

  if (pdf_document->document != NULL) {
    pdf_free_xref(pdf_document->document);
  }

  if (pdf_document->glyph_cache != NULL) {
    fz_free_glyph_cache(pdf_document->glyph_cache);
  }

  free(document->data);
  document->data = NULL;

error_ret:

  return false;
}

bool
pdf_document_free(zathura_document_t* document)
{
  if (document == NULL || document->data == NULL) {
    return false;
  }

  pdf_document_t* pdf_document = (pdf_document_t*) document->data;
  pdf_free_xref(pdf_document->document);
  fz_free_glyph_cache(pdf_document->glyph_cache);
  free(document->data);
  document->data = NULL;

  return true;
}

zathura_page_t*
pdf_page_get(zathura_document_t* document, unsigned int page)
{
  if (document == NULL || document->data == NULL) {
    return NULL;
  }

  pdf_document_t* pdf_document  = (pdf_document_t*) document->data;
  zathura_page_t* document_page = malloc(sizeof(zathura_page_t));

  if (document_page == NULL) {
    goto error_ret;
  }

  mupdf_page_t* mupdf_page = calloc(1, sizeof(mupdf_page_t));

  if (mupdf_page == NULL) {
    goto error_free;
  }

  document_page->document = document;
  document_page->data     = mupdf_page;

  /* load page */
  if (pdf_load_page(&(mupdf_page->page), pdf_document->document, page) != fz_okay) {
    goto error_free;
  }

  /* get page dimensions */
  document_page->width  = mupdf_page->page->mediabox.x1 - mupdf_page->page->mediabox.x0;
  document_page->height = mupdf_page->page->mediabox.y1 - mupdf_page->page->mediabox.y0;

  return document_page;

error_free:

  if (mupdf_page != NULL) {
    if (mupdf_page->page != NULL) {
      pdf_free_page(mupdf_page->page);
    }

    free(mupdf_page);
  }

  if (document_page != NULL) {
    free(document_page);
  }

error_ret:

  return NULL;
}

bool
pdf_page_free(zathura_page_t* page)
{
  if (page == NULL) {
    return false;
  }

  mupdf_page_t* mupdf_page = (mupdf_page_t*) page->data;
  pdf_free_page(mupdf_page->page);
  free(mupdf_page);
  free(page);

  return true;
}

girara_list_t*
pdf_page_search_text(zathura_page_t* page, const char* text)
{
  if (page == NULL || page->data == NULL || text == NULL) {
    goto error_ret;
  }

  /* mupdf_page_t* mupdf_page = (mupdf_page_t*) page->data; */

error_ret:

  return NULL;
}

girara_list_t*
pdf_page_links_get(zathura_page_t* page)
{
  return NULL;
}

girara_list_t*
pdf_page_form_fields_get(zathura_page_t* page)
{
  return NULL;
}

zathura_image_buffer_t*
pdf_page_render(zathura_page_t* page)
{
  if (page == NULL || page->data == NULL || page->document == NULL) {
    return NULL;
  }

  /* calculate sizes */
  unsigned int page_width  = page->document->scale * page->width;
  unsigned int page_height = page->document->scale * page->height;

  /* create image buffer */
  zathura_image_buffer_t* image_buffer = zathura_image_buffer_create(page_width, page_height);

  if (image_buffer == NULL) {
    return NULL;
  }

  pdf_document_t* pdf_document = (pdf_document_t*) page->document->data;
  mupdf_page_t* mupdf_page     = (mupdf_page_t*) page->data;

  /* render */
  fz_display_list* display_list = fz_new_display_list();
  fz_device* device             = fz_new_list_device(display_list);

  if (pdf_run_page(pdf_document->document, mupdf_page->page, device, fz_scale(page->document->scale, page->document->scale)) != fz_okay) {
    return NULL;
  }

  fz_free_device(device);

  fz_bbox bbox = { .x1 = page_width, .y1 = page_height };

  fz_pixmap* pixmap = fz_new_pixmap_with_rect(fz_device_rgb, bbox);
  fz_clear_pixmap_with_color(pixmap, 0xFF);

  device = fz_new_draw_device(pdf_document->glyph_cache, pixmap);
  fz_execute_display_list(display_list, device, fz_identity, bbox);
  fz_free_device(device);

  unsigned char* s = pixmap->samples;
  for (unsigned int y = 0; y < pixmap->h; y++) {
    for (unsigned int x = 0; x < pixmap->w; x++) {
      guchar* p = image_buffer->data + (pixmap->h - y - 1) *
        image_buffer->rowstride + x * 3;
      p[0] = s[0];
      p[1] = s[1];
      p[2] = s[2];
      s += pixmap->n;
    }
  }

  fz_drop_pixmap(pixmap);
  fz_free_display_list(display_list);

  return image_buffer;
}

#if HAVE_CAIRO
bool
pdf_page_render_cairo(zathura_page_t* page, cairo_t* cairo, bool GIRARA_UNUSED(printing))
{
  if (page == NULL || page->data == NULL || page->document == NULL) {
    return false;
  }

  cairo_surface_t* surface = cairo_get_target(cairo);
  if (surface == NULL) {
    return false;
  }

  unsigned int page_width  = cairo_image_surface_get_width(surface);
  unsigned int page_height = cairo_image_surface_get_height(surface);
  double scalex = ((double)page_width) / page->width,
         scaley = ((double)page_height) / page->height;

  pdf_document_t* pdf_document = (pdf_document_t*) page->document->data;
  mupdf_page_t* mupdf_page     = (mupdf_page_t*) page->data;

  /* render */
  fz_display_list* display_list = fz_new_display_list();
  fz_device* device             = fz_new_list_device(display_list);

  if (pdf_run_page(pdf_document->document, mupdf_page->page, device, fz_scale(scalex, scaley)) != fz_okay) {
    return false;
  }

  fz_free_device(device);

  fz_bbox bbox = { .x1 = page_width, .y1 = page_height };

  fz_pixmap* pixmap = fz_new_pixmap_with_rect(fz_device_rgb, bbox);
  fz_clear_pixmap_with_color(pixmap, 0xFF);

  device = fz_new_draw_device(pdf_document->glyph_cache, pixmap);
  fz_execute_display_list(display_list, device, fz_identity, bbox);
  fz_free_device(device);

  int rowstride        = cairo_image_surface_get_stride(surface);
  unsigned char* image = cairo_image_surface_get_data(surface);

  unsigned char *s = pixmap->samples;
  for (unsigned int y = 0; y < pixmap->h; y++) {
    for (unsigned int x = 0; x < pixmap->w; x++) {
      guchar* p = image + (pixmap->h - y - 1) * rowstride + x * 4;
      p[0] = s[0];
      p[1] = s[1];
      p[2] = s[2];
      s += pixmap->n;
    }
  }

  fz_drop_pixmap(pixmap);
  fz_free_display_list(display_list);

  return true;
}
#endif
