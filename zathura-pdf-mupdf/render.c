/* SPDX-License-Identifier: Zlib */

#include <glib.h>

#include "plugin.h"

static zathura_error_t
pdf_page_render_to_buffer(mupdf_document_t* mupdf_document, mupdf_page_t* mupdf_page,
			  unsigned char* image, int GIRARA_UNUSED(rowstride), int GIRARA_UNUSED(components),
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

  fz_irect irect = { .x1 = page_width, .y1 = page_height };
  fz_rect rect = { .x1 = page_width, .y1 = page_height };

  fz_display_list* display_list = fz_new_display_list(mupdf_page->ctx, rect);
  fz_device* device             = fz_new_list_device(mupdf_page->ctx, display_list);

  fz_try (mupdf_document->ctx) {
    fz_matrix m;
    m = fz_scale(scalex, scaley);
    fz_run_page(mupdf_document->ctx, mupdf_page->page, device, m, NULL);
  } fz_catch (mupdf_document->ctx) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  fz_close_device(mupdf_page->ctx, device);
  fz_drop_device(mupdf_page->ctx, device);

  fz_colorspace* colorspace = fz_device_bgr(mupdf_document->ctx);
  /* TODO: What are separations used for? */
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

zathura_image_buffer_t*
pdf_page_render(zathura_page_t* page, void* data, zathura_error_t* error)
{
  mupdf_page_t* mupdf_page = data;

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

zathura_error_t
pdf_page_render_cairo(zathura_page_t* page, void* data, cairo_t* cairo, bool GIRARA_UNUSED(printing))
{
  mupdf_page_t* mupdf_page = data;

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

