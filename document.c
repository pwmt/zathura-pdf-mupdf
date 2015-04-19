/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include <mupdf/fitz.h>
#include <mupdf/xps.h>
#include <mupdf/pdf.h>

#include "plugin.h"

zathura_error_t
pdf_document_open(zathura_document_t* document)
{
  zathura_error_t error = ZATHURA_ERROR_OK;
  if (document == NULL) {
    error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = calloc(1, sizeof(mupdf_document_t));
  if (mupdf_document == NULL) {
    error = ZATHURA_ERROR_OUT_OF_MEMORY;
    goto error_ret;
  }

  mupdf_document->ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
  if (mupdf_document->ctx == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  /* open document */
  const char* path     = zathura_document_get_path(document);
  const char* password = zathura_document_get_password(document);

  fz_try(mupdf_document->ctx){
    if (strstr(path, ".xps") != 0 || strstr(path, ".XPS") != 0 || strstr(path, ".rels") != 0) {
      mupdf_document->document = (fz_document*) xps_open_document(mupdf_document->ctx, (char*) path);
    } else {
      mupdf_document->document = (fz_document*) pdf_open_document(mupdf_document->ctx, (char*) path);
    }
  }
  fz_catch(mupdf_document->ctx){
    error = ZATHURA_ERROR_UNKNOWN;
    return error;
  }

  if (mupdf_document->document == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  /* authenticate if password is required and given */
  if (fz_needs_password(mupdf_document->ctx, mupdf_document->document) != 0) {
    if (password == NULL || fz_authenticate_password(mupdf_document->ctx, mupdf_document->document, (char*) password) == 0) {
      error = ZATHURA_ERROR_INVALID_PASSWORD;
      goto error_free;
    }
  }

  zathura_document_set_number_of_pages(document, fz_count_pages(mupdf_document->ctx, mupdf_document->document));
  zathura_document_set_data(document, mupdf_document);

  return error;

error_free:

  if (mupdf_document != NULL) {
    if (mupdf_document->document != NULL) {
      fz_drop_document(mupdf_document->ctx, mupdf_document->document);
    }
    if (mupdf_document->ctx != NULL) {
      fz_drop_context(mupdf_document->ctx);
    }

    free(mupdf_document);
  }

  zathura_document_set_data(document, NULL);

error_ret:

  return error;
}

zathura_error_t
pdf_document_free(zathura_document_t* document, mupdf_document_t* mupdf_document)
{
  if (document == NULL || mupdf_document == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  fz_drop_document(mupdf_document->ctx, mupdf_document->document);
  fz_drop_context(mupdf_document->ctx);
  free(mupdf_document);
  zathura_document_set_data(document, NULL);

  return ZATHURA_ERROR_OK;
}

zathura_error_t
pdf_document_save_as(zathura_document_t* document, mupdf_document_t*
    mupdf_document, const char* path)
{
  if (document == NULL || mupdf_document == NULL || path == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  fz_try (mupdf_document->ctx) {
    /* fz_write_document claims to accepts NULL as third argument but doesn't.
     * pdf_write_document does not check if the third arguments is NULL for some
     * options. */

    fz_write_options opts = { 0 }; /* just use the default options */
    fz_write_document(mupdf_document->ctx, mupdf_document->document, (char*) path, &opts);
  } fz_catch (mupdf_document->ctx) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  return ZATHURA_ERROR_OK;
}

