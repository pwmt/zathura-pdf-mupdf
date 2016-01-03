/* See LICENSE file for license and copyright information */

#include <stdlib.h>

#include "macros.h"
#include "plugin.h"
#include "internal.h"

zathura_error_t
pdf_document_open(zathura_document_t* document)
{
  if (document == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;

  /* Get path and password of file */
  char* path;
  if ((error = zathura_document_get_path(document, &path)) != ZATHURA_ERROR_OK) {
    return error;
  }

  char* password;
  if ((error = zathura_document_get_password(document, &password)) != ZATHURA_ERROR_OK) {
    return error;
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

  fz_try(mupdf_document->ctx){
    fz_register_document_handlers(mupdf_document->ctx);

    mupdf_document->document = fz_open_document(mupdf_document->ctx, path);
  }
  fz_catch(mupdf_document->ctx){
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  if (mupdf_document->document == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  /* authenticate if password is required and given */
  if (fz_needs_password(mupdf_document->ctx, mupdf_document->document) != 0) {
    if (password == NULL || fz_authenticate_password(mupdf_document->ctx, mupdf_document->document, (char*) password) == 0) {
      error = ZATHURA_ERROR_DOCUMENT_WRONG_PASSWORD;
      goto error_free;
    }
  }

  if (zathura_document_set_number_of_pages(document, fz_count_pages(mupdf_document->ctx, mupdf_document->document)) != ZATHURA_ERROR_OK) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  if (zathura_document_set_user_data(document, mupdf_document) != ZATHURA_ERROR_OK) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

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

error_ret:

  return error;
}

zathura_error_t
pdf_document_free(zathura_document_t* document)
{
  if (document == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  mupdf_document_t* mupdf_document;
  if (zathura_document_get_user_data(document, (void**) &mupdf_document) == ZATHURA_ERROR_OK) {
    if (mupdf_document != NULL) {
      fz_drop_document(mupdf_document->ctx, mupdf_document->document);
      fz_drop_context(mupdf_document->ctx);
      free(mupdf_document);
      zathura_document_set_user_data(document, NULL);
    }
  }

  return ZATHURA_ERROR_OK;
}

zathura_error_t
pdf_document_save_as(zathura_document_t* document, const char* path)
{
  if (document == NULL || path == NULL || strlen(path) == 0) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  mupdf_document_t* mupdf_document;
  if (zathura_document_get_user_data(document, (void**) &mupdf_document) != ZATHURA_ERROR_OK) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;

  fz_try (mupdf_document->ctx) {
    /* fz_write_document claims to accepts NULL as third argument but doesn't.
     * pdf_write_document does not check if the third arguments is NULL for some
     * options. */

    pdf_write_options opts = { 0 }; /* just use the default options */
    pdf_save_document(mupdf_document->ctx, (pdf_document*) mupdf_document->document, (char*) path, &opts);
  } fz_catch (mupdf_document->ctx) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  return error;
}
