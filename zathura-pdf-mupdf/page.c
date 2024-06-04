/* SPDX-License-Identifier: Zlib */

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

  g_mutex_lock(&mupdf_document->mutex);
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

  mupdf_page->bbox = fz_bound_page(mupdf_document->ctx, (fz_page*) mupdf_page->page);

  /* setup text */
  mupdf_page->extracted_text = false;

  mupdf_page->text = fz_new_stext_page(mupdf_page->ctx, mupdf_page->bbox);
  if (mupdf_page->text == NULL) {
    goto error_free;
  }
  g_mutex_unlock(&mupdf_document->mutex);

  zathura_page_set_data(page, mupdf_page);

  /* get page dimensions */
  zathura_page_set_width(page,  mupdf_page->bbox.x1 - mupdf_page->bbox.x0);
  zathura_page_set_height(page, mupdf_page->bbox.y1 - mupdf_page->bbox.y0);

  return ZATHURA_ERROR_OK;

error_free:
  g_mutex_unlock(&mupdf_document->mutex);

  pdf_page_clear(page, mupdf_page);

  return ZATHURA_ERROR_UNKNOWN;
}

zathura_error_t
pdf_page_clear(zathura_page_t* page, void* data)
{
  if (page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  mupdf_page_t* mupdf_page         = data;
  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  g_mutex_lock(&mupdf_document->mutex);
  if (mupdf_page != NULL) {
    if (mupdf_page->text != NULL) {
      fz_drop_stext_page(mupdf_page->ctx, mupdf_page->text);
    }

    if (mupdf_page->page != NULL) {
      fz_drop_page(mupdf_document->ctx, mupdf_page->page);
    }

    free(mupdf_page);
  }
  g_mutex_unlock(&mupdf_document->mutex);

  return ZATHURA_ERROR_UNKNOWN;
}


zathura_error_t
pdf_page_get_label(zathura_page_t* page, void* data, char** label)
{
  if (page == NULL || data == NULL || label == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  mupdf_page_t* mupdf_page = data;
  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  char buf[16];

  g_mutex_lock(&mupdf_document->mutex);
  fz_try(mupdf_page->ctx) {
    fz_page_label(mupdf_page->ctx, mupdf_page->page, buf, sizeof(buf));
  } fz_catch(mupdf_page->ctx) {
    g_mutex_unlock(&mupdf_document->mutex);
    return ZATHURA_ERROR_UNKNOWN;
  }
  g_mutex_unlock(&mupdf_document->mutex);

  // fz_page_label() may return an empty string if the label is undefined.
  if (buf[0] != '\0') {
      *label = g_strdup(buf);
  } else {
      *label = NULL;
  }

  return ZATHURA_ERROR_OK;
}
