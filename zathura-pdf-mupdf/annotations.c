/* See LICENSE file for license and copyright information */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "plugin.h"
#include "internal.h"
#include "macros.h"

static zathura_error_t mupdf_annotation_to_zathura_annotation(zathura_page_t*
    page, mupdf_document_t* mupdf_document, mupdf_page_t* mupdf_page, pdf_annot*
    mupdf_annot, zathura_annotation_t** annotation);

zathura_error_t
pdf_page_get_annotations(zathura_page_t* page, zathura_list_t** annotations)
{
  if (page == NULL || annotations == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;
  *annotations = NULL;

  zathura_document_t* document;
  if (zathura_page_get_document(page, &document) != ZATHURA_ERROR_OK || document == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
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

  pdf_annot* mupdf_annotation = pdf_first_annot(mupdf_document->ctx, (pdf_page*) mupdf_page->page);
  while (mupdf_annotation != NULL) {
    zathura_annotation_t* annotation;
    if (mupdf_annotation_to_zathura_annotation(page, mupdf_document, mupdf_page, mupdf_annotation, &annotation) !=
        ZATHURA_ERROR_OK) {
      mupdf_annotation = pdf_next_annot(mupdf_document->ctx, (pdf_page*) mupdf_page->page, mupdf_annotation);
      continue;
    }

    fz_rect bounding_box;
    pdf_bound_annot(mupdf_document->ctx, (pdf_page*) mupdf_page->page, mupdf_annotation, &bounding_box);

    zathura_rectangle_t position = {
      {bounding_box.x0, bounding_box.y0},
      {bounding_box.x1, bounding_box.y1}
    };

    if ((error = zathura_annotation_set_position(annotation, position)) != ZATHURA_ERROR_OK) {
      break;
    }

    *annotations = zathura_list_append(*annotations, annotation);

    /* next annot */
    mupdf_annotation = pdf_next_annot(mupdf_document->ctx, (pdf_page*) mupdf_page->page, mupdf_annotation);
  }

  return error;

error_out:

  return error;
}

static zathura_error_t
mupdf_annotation_to_zathura_annotation(zathura_page_t* page, mupdf_document_t*
    mupdf_document, mupdf_page_t* mupdf_page, pdf_annot* mupdf_annotation,
    zathura_annotation_t** annotation)
{
  fz_annot_type mupdf_type = pdf_annot_type(mupdf_document->ctx, mupdf_annotation);
  zathura_annotation_type_t zathura_type = ZATHURA_ANNOTATION_UNKNOWN;

  zathura_error_t error = ZATHURA_ERROR_OK;

  typedef struct annotation_type_mapping_s {
    fz_annot_type mupdf;
    zathura_annotation_type_t zathura;
  } annotation_type_mapping_t;

  annotation_type_mapping_t type_mapping[] = {
    { FZ_ANNOT_TEXT,           ZATHURA_ANNOTATION_TEXT },
    { FZ_ANNOT_FREETEXT,       ZATHURA_ANNOTATION_FREE_TEXT },
    { FZ_ANNOT_LINE,           ZATHURA_ANNOTATION_LINE },
    { FZ_ANNOT_SQUARE,         ZATHURA_ANNOTATION_SQUARE },
    { FZ_ANNOT_CIRCLE,         ZATHURA_ANNOTATION_CIRCLE },
    { FZ_ANNOT_POLYGON,        ZATHURA_ANNOTATION_POLYGON },
    { FZ_ANNOT_POLYLINE,       ZATHURA_ANNOTATION_POLY_LINE },
    { FZ_ANNOT_HIGHLIGHT,      ZATHURA_ANNOTATION_HIGHLIGHT },
    { FZ_ANNOT_UNDERLINE,      ZATHURA_ANNOTATION_UNDERLINE },
    { FZ_ANNOT_SQUIGGLY,       ZATHURA_ANNOTATION_SQUIGGLY },
    { FZ_ANNOT_STRIKEOUT,      ZATHURA_ANNOTATION_STRIKE_OUT },
    { FZ_ANNOT_STAMP,          ZATHURA_ANNOTATION_STAMP },
    { FZ_ANNOT_CARET,          ZATHURA_ANNOTATION_CARET },
    { FZ_ANNOT_INK,            ZATHURA_ANNOTATION_INK },
    { FZ_ANNOT_POPUP,          ZATHURA_ANNOTATION_POPUP },
    { FZ_ANNOT_FILEATTACHMENT, ZATHURA_ANNOTATION_FILE_ATTACHMENT },
    { FZ_ANNOT_SOUND,          ZATHURA_ANNOTATION_SOUND },
    { FZ_ANNOT_MOVIE,          ZATHURA_ANNOTATION_MOVIE },
    { FZ_ANNOT_WIDGET,         ZATHURA_ANNOTATION_WIDGET },
    { FZ_ANNOT_SCREEN,         ZATHURA_ANNOTATION_SCREEN },
    { FZ_ANNOT_PRINTERMARK,    ZATHURA_ANNOTATION_PRINTER_MARK },
    { FZ_ANNOT_TRAPNET,        ZATHURA_ANNOTATION_TRAP_NET },
    { FZ_ANNOT_WATERMARK,      ZATHURA_ANNOTATION_WATERMARK },
    { FZ_ANNOT_3D,             ZATHURA_ANNOTATION_3D }
  };

  for (unsigned int i = 0; i < LENGTH(type_mapping); i++) {
    if (type_mapping[i].mupdf == mupdf_type) {
      zathura_type = type_mapping[i].zathura;
      break;
    }
  }

  /* create new annotation */
  if ((error = zathura_annotation_new(annotation, zathura_type)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  /* set general properties */
  char* content = pdf_annot_contents(mupdf_document->ctx, (pdf_document*) mupdf_document->document, mupdf_annotation);
  if (content != NULL && (error = zathura_annotation_set_content(*annotation, content) != ZATHURA_ERROR_OK)) {
    goto error_free;
  }

  return error;

error_free:

    zathura_annotation_free(*annotation);

error_out:

    return error;
}
