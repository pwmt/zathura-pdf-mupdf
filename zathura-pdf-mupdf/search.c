/* See LICENSE file for license and copyright information */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "plugin.h"
#include "internal.h"
#include "utils.h"

#define N_SEARCH_RESULTS 512

zathura_error_t pdf_page_search_text(zathura_page_t* page, const char* text,
    zathura_search_flag_t flags, zathura_list_t** results)
{
  (void) flags;

  if (page == NULL || text == NULL || strlen(text) == 0 || results == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;
  *results = NULL;

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

  /* extract text */
  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document, mupdf_page);
  }

  fz_quad* hit_bbox = fz_malloc_array(mupdf_page->ctx, N_SEARCH_RESULTS, fz_quad);
  int num_results = fz_search_stext_page(mupdf_page->ctx, mupdf_page->text,
      (char*) text, NULL, hit_bbox, N_SEARCH_RESULTS);

  if (num_results == 0) {
    error = ZATHURA_ERROR_SEARCH_NO_RESULTS;
    goto error_out;
  }

  for (int i = 0; i < num_results/2; i++) { // TODO: Understand the div 2
    zathura_rectangle_t* rectangle = calloc(1, sizeof(zathura_rectangle_t));
    if (rectangle == NULL) {
      error = ZATHURA_ERROR_OUT_OF_MEMORY;
      goto error_free;
    }

    fz_rect r = fz_rect_from_quad(hit_bbox[i]);
    rectangle->p1.x = r.x0;
    rectangle->p1.y = r.y1;
    rectangle->p2.x = r.x1;
    rectangle->p2.y = r.y0;

    *results = zathura_list_prepend(*results, rectangle);
  }

  *results = zathura_list_reverse(*results);

  return error;

error_free:

  zathura_list_free_full(*results, free);
  *results = NULL;

error_out:

  return error;
}
