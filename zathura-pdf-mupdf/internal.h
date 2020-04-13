/* See LICENSE file for license and copyright information */ 

#ifndef INTERNAL_H
#define INTERNAL_H

#define	__USE_POSIX 1

#include <stdbool.h>

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

typedef struct mupdf_document_s
{
  fz_context* ctx; /**< Context */
  fz_document* document; /**< mupdf document */
} mupdf_document_t;

typedef struct mupdf_page_s
{
  fz_page* page; /**< Reference to the mupdf page */
  fz_context* ctx; /**< Context */
  fz_stext_page* text; /**< Page text */
  fz_rect bbox; /**< Bbox */
  bool extracted_text; /**< If text has already been extracted */
} mupdf_page_t;

#endif /* INTERNAL_H */
