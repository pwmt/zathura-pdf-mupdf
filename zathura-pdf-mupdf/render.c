/* See LICENSE file for license and copyright information */

#if HAVE_CAIRO
#include <cairo.h>
#endif

#include "plugin.h"
#include "internal.h"

static zathura_error_t
pdf_page_render_to_buffer(mupdf_document_t* mupdf_document, mupdf_page_t*
    mupdf_page, unsigned char* image, unsigned int page_width, unsigned int
    page_height, double scalex, double scaley)
{
  if (mupdf_document == NULL ||
      mupdf_document->ctx == NULL ||
      mupdf_page == NULL ||
      mupdf_page->page == NULL ||
      image == NULL) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  fz_irect irect = { .x1 = page_width, .y1 = page_height };
  fz_rect rect = { .x1 = page_width, .y1 = page_height };

  fz_display_list* display_list = fz_new_display_list(mupdf_page->ctx, rect);
  fz_device* device             = fz_new_list_device(mupdf_page->ctx, display_list);

  fz_try (mupdf_document->ctx) {
    fz_matrix m = fz_scale(scalex, scaley);
    pdf_run_page_contents(mupdf_document->ctx, (pdf_page*) mupdf_page->page, device, m, NULL);
  } fz_catch (mupdf_document->ctx) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  fz_colorspace* colorspace = fz_device_rgb(mupdf_document->ctx);
  fz_pixmap* pixmap = fz_new_pixmap_with_bbox_and_data(mupdf_page->ctx, colorspace, irect, NULL, 1, image);
  fz_clear_pixmap_with_value(mupdf_page->ctx, pixmap, 0xFF);

  device = fz_new_draw_device(mupdf_page->ctx, fz_identity, pixmap);
  fz_run_display_list(mupdf_page->ctx, display_list, device, fz_identity, rect, NULL);

  fz_close_device(mupdf_page->ctx, device);
  fz_drop_device(mupdf_page->ctx, device);

  fz_drop_pixmap(mupdf_page->ctx, pixmap);
  fz_drop_display_list(mupdf_page->ctx, display_list);

  return ZATHURA_ERROR_OK;
}

#ifdef HAVE_CAIRO
zathura_error_t pdf_page_render_cairo(zathura_page_t* page, cairo_t* cairo,
    double scale, int rotation, int flags)
{
  (void) rotation;
  (void) flags;

  if (page == NULL || cairo == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;

  cairo_surface_t* surface = cairo_get_target(cairo);
  if (surface == NULL ||
      cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS ||
      cairo_surface_get_type(surface) != CAIRO_SURFACE_TYPE_IMAGE) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  zathura_document_t* document;
  if (zathura_page_get_document(page, &document) != ZATHURA_ERROR_OK || document == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  mupdf_document_t* mupdf_document;
  if (zathura_document_get_user_data(document, (void**) &mupdf_document) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_page_t* mupdf_page;
  if ((error = zathura_page_get_user_data(page, (void**) &mupdf_page)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  unsigned int page_width  = cairo_image_surface_get_width(surface);
  unsigned int page_height = cairo_image_surface_get_height(surface);

  unsigned char* image = cairo_image_surface_get_data(surface);

  return pdf_page_render_to_buffer(mupdf_document, mupdf_page, image,
      page_width, page_height, scale, scale);

error_out:

  return error;
}
#endif
