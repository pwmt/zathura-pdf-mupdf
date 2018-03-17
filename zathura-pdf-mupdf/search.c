/* See LICENSE file for license and copyright information */

#define N_SEARCH_RESULTS 512

#include <glib.h>

#include "plugin.h"
#include "utils.h"

girara_list_t*
pdf_page_search_text(zathura_page_t* page, void* data, const char* text, zathura_error_t* error)
{
  mupdf_page_t* mupdf_page = data;

  if (page == NULL || text == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL || mupdf_page == NULL || mupdf_page->text == NULL) {
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);;

  girara_list_t* list = girara_list_new2(g_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  /* extract text */
  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document, mupdf_page);
  }

  fz_rect* hit_bbox = fz_malloc_array(mupdf_page->ctx, N_SEARCH_RESULTS, sizeof(fz_rect));
  int num_results = fz_search_stext_page(mupdf_page->ctx, mupdf_page->text,
      text, hit_bbox, N_SEARCH_RESULTS);

  for (int i = 0; i < num_results; i++) {
    zathura_rectangle_t* rectangle = g_malloc0(sizeof(zathura_rectangle_t));

    rectangle->x1 = hit_bbox[i].x0;
    rectangle->x2 = hit_bbox[i].x1;
    rectangle->y1 = hit_bbox[i].y0;
    rectangle->y2 = hit_bbox[i].y1;

    girara_list_append(list, rectangle);
  }

  fz_free(mupdf_page->ctx, hit_bbox);

  return list;

error_free:

  if (list != NULL ) {
    girara_list_free(list);
  }

error_ret:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  return NULL;
}

