/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include <glib.h>

#include "plugin.h"

static zathura_error_t
pdf_page_render_to_buffer(mupdf_document_t* mupdf_document, mupdf_page_t* mupdf_page,
			  unsigned char* image, int rowstride, int components,
			  unsigned int page_width, unsigned int page_height,
			  double scalex, double scaley)
{
  if (mupdf_document == NULL ||
      mupdf_document->ctx == NULL ||
      mupdf_page == NULL ||
      mupdf_page->page == NULL ||
      image == NULL) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  fz_display_list* display_list = fz_new_display_list(mupdf_page->ctx);
  fz_device* device             = fz_new_list_device(mupdf_page->ctx, display_list);

  fz_try (mupdf_document->ctx) {
    fz_matrix m;
    fz_scale(&m, scalex, scaley);
    fz_run_page(mupdf_document->ctx, mupdf_page->page, device, &m, NULL);
  } fz_catch (mupdf_document->ctx) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  fz_drop_device(mupdf_page->ctx, device);

  fz_irect irect = { .x1 = page_width, .y1 = page_height };
  fz_rect rect = { .x1 = page_width, .y1 = page_height };

  fz_colorspace* colorspace = fz_device_rgb(mupdf_document->ctx);
  fz_pixmap* pixmap = fz_new_pixmap_with_bbox(mupdf_page->ctx, colorspace, &irect);
  fz_clear_pixmap_with_value(mupdf_page->ctx, pixmap, 0xFF);

  device = fz_new_draw_device(mupdf_page->ctx, pixmap);
  fz_run_display_list(mupdf_page->ctx, display_list, device, &fz_identity, &rect, NULL);
  fz_drop_device(mupdf_page->ctx, device);

  unsigned char* s = fz_pixmap_samples(mupdf_page->ctx, pixmap);
  unsigned int n   = fz_pixmap_components(mupdf_page->ctx, pixmap);
  for (unsigned int y = 0; y < fz_pixmap_height(mupdf_page->ctx, pixmap); y++) {
    for (unsigned int x = 0; x < fz_pixmap_width(mupdf_page->ctx, pixmap); x++) {
      guchar* p = image + y * rowstride + x * components;
      p[0] = s[2];
      p[1] = s[1];
      p[2] = s[0];
      s += n;
    }
  }

  fz_drop_pixmap(mupdf_page->ctx, pixmap);
  fz_drop_display_list(mupdf_page->ctx, display_list);

  return ZATHURA_ERROR_OK;
}

zathura_image_buffer_t*
pdf_page_render(zathura_page_t* page, mupdf_page_t* mupdf_page, zathura_error_t* error)
{
  if (page == NULL || mupdf_page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    return NULL;
  }

  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL) {
    return NULL;
  }

  /* calculate sizes */
  double scalex            = zathura_document_get_scale(document);
  double scaley            = scalex;
  unsigned int page_width  = scalex * zathura_page_get_width(page);
  unsigned int page_height = scaley * zathura_page_get_height(page);

  /* create image buffer */
  zathura_image_buffer_t* image_buffer = zathura_image_buffer_create(page_width, page_height);

  if (image_buffer == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    return NULL;
  }

  int rowstride        = image_buffer->rowstride;
  unsigned char* image = image_buffer->data;

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  zathura_error_t error_render = pdf_page_render_to_buffer(mupdf_document, mupdf_page, image, rowstride, 3,
						page_width, page_height, scalex, scaley);

  if (error_render != ZATHURA_ERROR_OK) {
    zathura_image_buffer_free(image_buffer);
    if (error != NULL) {
      *error = error_render;
    }
    return NULL;
  }

  return image_buffer;
}

#if HAVE_CAIRO
zathura_error_t
pdf_page_render_cairo(zathura_page_t* page, mupdf_page_t* mupdf_page, cairo_t* cairo, bool GIRARA_UNUSED(printing))
{
  if (page == NULL || mupdf_page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  cairo_surface_t* surface = cairo_get_target(cairo);
  if (surface == NULL ||
      cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS ||
      cairo_surface_get_type(surface) != CAIRO_SURFACE_TYPE_IMAGE) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  unsigned int page_width  = cairo_image_surface_get_width(surface);
  unsigned int page_height = cairo_image_surface_get_height(surface);

  double scalex = ((double) page_width) / zathura_page_get_width(page);
  double scaley = ((double) page_height) /zathura_page_get_height(page);

  int rowstride        = cairo_image_surface_get_stride(surface);
  unsigned char* image = cairo_image_surface_get_data(surface);

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  return pdf_page_render_to_buffer(mupdf_document, mupdf_page, image, rowstride, 4,
				   page_width, page_height, scalex, scaley);
}
#endif

