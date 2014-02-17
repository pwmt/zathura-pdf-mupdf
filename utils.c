/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include "utils.h"

void
mupdf_page_extract_text(mupdf_document_t* mupdf_document, mupdf_page_t* mupdf_page) {
  if (mupdf_document == NULL || mupdf_document->ctx == NULL || mupdf_page == NULL ||
      mupdf_page->sheet == NULL || mupdf_page->text == NULL) {
    return;
  }

  fz_device* text_device = NULL;

  fz_try (mupdf_page->ctx) {
    text_device = fz_new_text_device(mupdf_page->ctx, mupdf_page->sheet, mupdf_page->text);
    fz_matrix ctm;
    fz_scale(&ctm, 1.0, 1.0);
    fz_run_page(mupdf_document->document, mupdf_page->page, text_device, &ctm, NULL);
  } fz_always (mupdf_document->ctx) {
    fz_free_device(text_device);
  } fz_catch(mupdf_document->ctx) {
  }

  mupdf_page->extracted_text = true;
}
