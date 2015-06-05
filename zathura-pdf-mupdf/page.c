/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include "plugin.h"

zathura_error_t
pdf_page_init(zathura_page_t* page)
{
  if (page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);
  mupdf_page_t* mupdf_page         = calloc(1, sizeof(mupdf_page_t));
  unsigned int index               = zathura_page_get_index(page);

  if (mupdf_page == NULL) {
    return  ZATHURA_ERROR_OUT_OF_MEMORY;
  }

  zathura_page_set_data(page, mupdf_page);

  mupdf_page->ctx = mupdf_document->ctx;
  if (mupdf_page->ctx == NULL) {
    goto error_free;
  }

  /* load page */
  fz_try (mupdf_page->ctx) {
    mupdf_page->page = fz_load_page(mupdf_document->ctx, mupdf_document->document, index);
  } fz_catch (mupdf_page->ctx) {
    goto error_free;
  }

  fz_bound_page(mupdf_document->ctx, (fz_page*) mupdf_page->page, &mupdf_page->bbox);

  /* get page dimensions */
  zathura_page_set_width(page,  mupdf_page->bbox.x1 - mupdf_page->bbox.x0);
  zathura_page_set_height(page, mupdf_page->bbox.y1 - mupdf_page->bbox.y0);

  /* setup text */
  mupdf_page->extracted_text = false;

  mupdf_page->text = fz_new_text_page(mupdf_page->ctx);
  if (mupdf_page->text == NULL) {
    goto error_free;
  }

  mupdf_page->sheet = fz_new_text_sheet(mupdf_page->ctx);
  if (mupdf_page->sheet == NULL) {
    goto error_free;
  }

  return ZATHURA_ERROR_OK;

error_free:

  pdf_page_clear(page, mupdf_page);

  return ZATHURA_ERROR_UNKNOWN;
}

zathura_error_t
pdf_page_clear(zathura_page_t* page, mupdf_page_t* mupdf_page)
{
  if (page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  if (mupdf_page != NULL) {
    if (mupdf_page->text != NULL) {
      fz_drop_text_page(mupdf_page->ctx, mupdf_page->text);
    }

    if (mupdf_page->sheet != NULL) {
      fz_drop_text_sheet(mupdf_page->ctx, mupdf_page->sheet);
    }

    if (mupdf_page->page != NULL) {
      fz_drop_page(mupdf_document->ctx, mupdf_page->page);
    }

    free(mupdf_page);
  }

  return ZATHURA_ERROR_UNKNOWN;
}

