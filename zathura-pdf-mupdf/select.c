/* SPDX-License-Identifier: Zlib */

#include <mupdf/pdf.h>

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

  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document, mupdf_page);
  }

  fz_point a = { rectangle.x1, rectangle.y1 };
  fz_point b = { rectangle.x2, rectangle.y2 };

#ifdef _WIN32
  return fz_copy_selection(mupdf_page->ctx, mupdf_page->text, a, b, 1);
#else
  return fz_copy_selection(mupdf_page->ctx, mupdf_page->text, a, b, 0);
#endif

error_ret:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  return NULL;
}
