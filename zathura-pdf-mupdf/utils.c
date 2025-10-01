/* SPDX-License-Identifier: Zlib */

#include "utils.h"

void mupdf_page_extract_text(mupdf_document_t* mupdf_document, mupdf_page_t* mupdf_page) {
  if (mupdf_document == NULL || mupdf_document->ctx == NULL || mupdf_page == NULL || mupdf_page->text == NULL) {
    return;
  }

  fz_device* volatile text_device = NULL;

  fz_try(mupdf_page->ctx) {
    fz_stext_options stext_options;
    stext_options.flags = FZ_STEXT_PRESERVE_IMAGES;
    text_device = fz_new_stext_device(mupdf_page->ctx, mupdf_page->text, &stext_options);

    fz_matrix ctm;
    ctm = fz_scale(1.0, 1.0);
    fz_run_page(mupdf_page->ctx, mupdf_page->page, text_device, ctm, NULL);
  }
  fz_always(mupdf_document->ctx) {
    fz_close_device(mupdf_page->ctx, text_device);
    fz_drop_device(mupdf_page->ctx, text_device);
  }
  fz_catch(mupdf_document->ctx) {}

  mupdf_page->extracted_text = true;
}
