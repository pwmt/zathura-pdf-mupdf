/* See LICENSE file for license and copyright information */

#include <stdio.h>

#include "plugin.h"
#include "internal.h"
#include "utils.h"

zathura_error_t
pdf_page_get_text(zathura_page_t* page, char** text)
{
  if (page == NULL || text == NULL) {
      return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;

  unsigned int width;
  if ((error = zathura_page_get_width(page, &width)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  unsigned int height;
  if ((error = zathura_page_get_height(page, &height)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  zathura_rectangle_t rect = {
    {0, 0},
    {width, height}
  };

  return pdf_page_get_selected_text(page, text, rect);

error_out:

  return error;
}

zathura_error_t
pdf_page_get_selected_text(zathura_page_t* page, char** text, zathura_rectangle_t rectangle)
{
  if (page == NULL || text == NULL) {
      return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;

  zathura_document_t* document;
  if ((error = zathura_page_get_document(page, &document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_document_t* mupdf_document;
  if ((error = zathura_document_get_data(document, (void**) &mupdf_document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_page_t* mupdf_page;
  if ((error = zathura_page_get_data(page, (void**) &mupdf_page)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  /* extract text */
  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document, mupdf_page);
  }

  fz_rect rect = { rectangle.p1.x, rectangle.p1.y, rectangle.p2.x, rectangle.p2.y };

  *text = fz_copy_selection(mupdf_page->ctx, mupdf_page->text, rect);

error_out:

  return error;
}
