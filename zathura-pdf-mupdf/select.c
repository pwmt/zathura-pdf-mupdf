/* SPDX-License-Identifier: Zlib */

#define MAX_QUADS 1000

#include "plugin.h"
#include "utils.h"

char*
pdf_page_get_text(zathura_page_t* page, void* data, zathura_rectangle_t rectangle, zathura_error_t* error)
{
  mupdf_page_t* mupdf_page = data;

  if (page == NULL || mupdf_page == NULL || mupdf_page->text == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);
  g_mutex_lock(&mupdf_document->mutex);

  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document, mupdf_page);
  }

  fz_point a = { rectangle.x1, rectangle.y1 };
  fz_point b = { rectangle.x2, rectangle.y2 };

  char* ret = NULL;
#ifdef _WIN32
  ret = fz_copy_selection(mupdf_page->ctx, mupdf_page->text, a, b, 1);
#else
  ret = fz_copy_selection(mupdf_page->ctx, mupdf_page->text, a, b, 0);
#endif
  g_mutex_unlock(&mupdf_document->mutex);
  return ret;

error_ret:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  return NULL;
}

girara_list_t*
pdf_page_get_selection(zathura_page_t* page, void* data, zathura_rectangle_t rectangle, zathura_error_t* error) {

  mupdf_page_t* mupdf_page = data;

  if (page == NULL || mupdf_page == NULL || mupdf_page->text == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);
  g_mutex_lock(&mupdf_document->mutex);

  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document, mupdf_page);
  }

  fz_point a = { rectangle.x1, rectangle.y1 };
  fz_point b = { rectangle.x2, rectangle.y2 };

  girara_list_t* list = girara_list_new2(g_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  fz_quad* hits = fz_malloc_array(mupdf_page->ctx, MAX_QUADS, fz_quad);
  int num_results = fz_highlight_selection(mupdf_page->ctx, mupdf_page->text, a, b, hits, MAX_QUADS);

  fz_rect r;
  for (int i = 0; i < num_results; i++) {
    zathura_rectangle_t* inner_rectangle = g_malloc0(sizeof(zathura_rectangle_t));

    r = fz_rect_from_quad(hits[i]);
    inner_rectangle->x1 = r.x0;
    inner_rectangle->x2 = r.x1;
    inner_rectangle->y1 = r.y0;
    inner_rectangle->y2 = r.y1;

    girara_list_append(list, inner_rectangle);
  }

  fz_free(mupdf_page->ctx, hits);
  g_mutex_unlock(&mupdf_document->mutex);

  return list;

error_free:
  g_mutex_unlock(&mupdf_document->mutex);

  if (list != NULL ) {
      girara_list_free(list);
  }

error_ret:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  return NULL;
}

