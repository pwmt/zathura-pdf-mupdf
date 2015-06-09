/* See LICENSE file for license and copyright information */

#include <stdlib.h>

#include "plugin.h"
#include "internal.h"
#include "utils.h"

typedef struct mupdf_image_s {
  mupdf_page_t* page;
  fz_image* image;
} mupdf_image_t;

#if HAVE_CAIRO
zathura_error_t
pdf_image_get_cairo_surface(zathura_image_t* image, cairo_surface_t** surface)
{
  if (image == NULL || surface == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;

  mupdf_image_t* mupdf_image;
  if ((error = zathura_image_get_user_data(image, (void**) &mupdf_image)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_page_t* mupdf_page = mupdf_image->page;

  fz_pixmap* pixmap = fz_new_pixmap_from_image(mupdf_page->ctx, mupdf_image->image, 0, 0);
  if (pixmap == NULL) {
    goto error_free;
  }

  *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, mupdf_image->image->w, mupdf_image->image->h);
  if (*surface == NULL) {
    error = ZATHURA_ERROR_OUT_OF_MEMORY;
    goto error_free;
  }

  unsigned char* surface_data = cairo_image_surface_get_data(*surface);
  int rowstride = cairo_image_surface_get_stride(*surface);

  unsigned char* s = fz_pixmap_samples(mupdf_page->ctx, pixmap);
  unsigned int n   = fz_pixmap_components(mupdf_page->ctx, pixmap);

  for (int y = 0; y < fz_pixmap_height(mupdf_page->ctx, pixmap); y++) {
    for (int x = 0; x < fz_pixmap_width(mupdf_page->ctx, pixmap); x++) {
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

  return error;

error_free:

  if (pixmap != NULL) {
    fz_drop_pixmap(mupdf_page->ctx, pixmap);
  }

  if (surface != NULL) {
    cairo_surface_destroy(*surface);
  }

error_out:

  return error;
}
#endif

zathura_error_t
pdf_page_get_images(zathura_page_t* page, zathura_list_t** images)
{
  if (page == NULL || images == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;
  *images = NULL;

  zathura_document_t* document;
  if ((error = zathura_page_get_document(page, &document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_document_t* mupdf_document;
  if ((error = zathura_document_get_user_data(document, (void**) &mupdf_document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_page_t* mupdf_page;
  if ((error = zathura_page_get_user_data(page, (void**) &mupdf_page)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  /* Extract images */
  mupdf_page_extract_text(mupdf_document, mupdf_page);

  fz_page_block* block;
  for (block = mupdf_page->text->blocks; block < mupdf_page->text->blocks + mupdf_page->text->len; block++) {
    if (block->type == FZ_PAGE_BLOCK_IMAGE) {
      fz_image_block *image_block = block->u.image;

      zathura_rectangle_t position = {
        { image_block->bbox.x0, image_block->bbox.y0 },
        { image_block->bbox.x1, image_block->bbox.y1 }
      };

      zathura_image_t* zathura_image;
      if (zathura_image_new(&zathura_image, position) != ZATHURA_ERROR_OK) {
        error = ZATHURA_ERROR_OUT_OF_MEMORY;
        goto error_free;
      }

      #if HAVE_CAIRO
      if (zathura_image_set_get_cairo_surface_function(zathura_image,
            pdf_image_get_cairo_surface) != ZATHURA_ERROR_OK) {
        zathura_image_free(zathura_image);
        error = ZATHURA_ERROR_UNKNOWN;
        goto error_free;
      }
      #endif

      mupdf_image_t* mupdf_image = calloc(1, sizeof(mupdf_image_t));
      if (mupdf_image == NULL) {
        error = ZATHURA_ERROR_OUT_OF_MEMORY;
        goto error_free;
      }

      mupdf_image->page  = mupdf_page;
      mupdf_image->image = image_block->image;

      if (zathura_image_set_user_data(zathura_image, mupdf_image, free) != ZATHURA_ERROR_OK) {
        zathura_image_free(zathura_image);
        error = ZATHURA_ERROR_UNKNOWN;
        goto error_free;
      }

      *images = zathura_list_append(*images, zathura_image);
    }
  }

  return error;

error_free:

  zathura_list_free_full(*images, (GDestroyNotify) zathura_image_free);

error_out:

  return error;
}
