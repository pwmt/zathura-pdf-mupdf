/* See LICENSE file for license and copyright information */

#include <glib.h>
#include <girara/datastructures.h>
#include <mupdf/pdf.h>

#include "plugin.h"
#include "utils.h"

static void pdf_zathura_image_free(zathura_image_t* image);

girara_list_t*
pdf_page_images_get(zathura_page_t* page, void* data, zathura_error_t* error)
{
  mupdf_page_t* mupdf_page = data;

  if (page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  girara_list_t* list = NULL;
  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL) {
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  /* Setup image list */
  list = girara_list_new();
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  girara_list_set_free_function(list, (girara_free_function_t) pdf_zathura_image_free);

  /* Extract images */
  mupdf_page_extract_text(mupdf_document, mupdf_page);

  fz_stext_block* block;
  for (block = mupdf_page->text->first_block; block; block = block->next) {
    if (block->type == FZ_STEXT_BLOCK_IMAGE) {
      zathura_image_t* zathura_image = g_malloc(sizeof(zathura_image_t));

      zathura_image->position.x1 = block->bbox.x0;
      zathura_image->position.y1 = block->bbox.y0;
      zathura_image->position.x2 = block->bbox.x1;
      zathura_image->position.y2 = block->bbox.y1;
      zathura_image->data        = block->u.i.image;

      girara_list_append(list, zathura_image);
    }
  }

  return list;

error_free:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  if (list != NULL) {
    girara_list_free(list);
  }

error_ret:

  return NULL;
}

cairo_surface_t*
pdf_page_image_get_cairo(zathura_page_t* page, void* data,
    zathura_image_t* image, zathura_error_t* error)
{
  mupdf_page_t* mupdf_page = data;

  if (page == NULL || mupdf_page == NULL || image == NULL || image->data == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  fz_image* mupdf_image = (fz_image*) image->data;

  fz_pixmap* pixmap = NULL;
  cairo_surface_t* surface = NULL;

  pixmap = fz_get_pixmap_from_image(mupdf_page->ctx, mupdf_image, NULL, NULL, 0, 0);
  if (pixmap == NULL) {
    goto error_free;
  }

  surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, mupdf_image->w, mupdf_image->h);
  if (surface == NULL) {
    goto error_free;
  }

  unsigned char* surface_data = cairo_image_surface_get_data(surface);
  int rowstride = cairo_image_surface_get_stride(surface);

  unsigned char* s = fz_pixmap_samples(mupdf_page->ctx, pixmap);
  unsigned int n   = fz_pixmap_components(mupdf_page->ctx, pixmap);

  for (unsigned int y = 0; y < fz_pixmap_height(mupdf_page->ctx, pixmap); y++) {
    for (unsigned int x = 0; x < fz_pixmap_width(mupdf_page->ctx, pixmap); x++) {
      guchar* p = surface_data + y * rowstride + x * 4;

      // RGB
      if (n == 4) {
        p[0] = s[2];
        p[1] = s[1];
        p[2] = s[0];
      // Gray-scale or mask
      } else {
        p[0] = s[0];
        p[1] = s[0];
        p[2] = s[0];
      }
      s += n;
    }
  }

  fz_drop_pixmap(mupdf_page->ctx, pixmap);

  return surface;

error_free:

  if (pixmap != NULL) {
    fz_drop_pixmap(mupdf_page->ctx, pixmap);
  }

  if (surface != NULL) {
    cairo_surface_destroy(surface);
  }

error_ret:

  return NULL;
}

static void
pdf_zathura_image_free(zathura_image_t* image)
{
  if (image == NULL) {
    return;
  }

  g_free(image);
}
