/* SPDX-License-Identifier: Zlib */

#define N_SEARCH_RESULTS 512

#include <glib.h>

#include "plugin.h"
#include "utils.h"

girara_list_t* pdf_page_search_text(zathura_page_t* page, void* data, const char* text, zathura_error_t* error) {
  if (page == NULL || text == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  mupdf_page_t* mupdf_page     = data;
  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL || mupdf_page == NULL || mupdf_page->text == NULL) {
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);
  ;

  girara_list_t* list = girara_list_new_with_free(g_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  g_mutex_lock(&mupdf_document->mutex);

  /* extract text */
  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document, mupdf_page);
  }

  fz_quad* hit_bbox = fz_malloc_array(mupdf_page->ctx, N_SEARCH_RESULTS, fz_quad);
  int num_results   = fz_search_stext_page(mupdf_page->ctx, mupdf_page->text, text, NULL, hit_bbox, N_SEARCH_RESULTS);

  fz_rect r;
  for (int i = 0; i < num_results; i++) {
    zathura_rectangle_t* rectangle = g_malloc0(sizeof(zathura_rectangle_t));

    r             = fz_rect_from_quad(hit_bbox[i]);
    rectangle->x1 = r.x0;
    rectangle->x2 = r.x1;
    rectangle->y1 = r.y0;
    rectangle->y2 = r.y1;

    girara_list_append(list, rectangle);
  }

  fz_free(mupdf_page->ctx, hit_bbox);
  g_mutex_unlock(&mupdf_document->mutex);

  return list;

error_free:

  if (list != NULL) {
    girara_list_free(list);
  }

error_ret:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  return NULL;
}
