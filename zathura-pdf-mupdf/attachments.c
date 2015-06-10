/* See LICENSE file for license and copyright information */

#include <stdio.h>

#include "plugin.h"
#include "internal.h"
#include "macros.h"

static zathura_attachment_t* parse_mupdf_attachment(mupdf_document_t*
    mupdf_document, pdf_obj* file);
static zathura_error_t pdf_attachment_save(zathura_attachment_t* attachment,
    const char* path, void* user_data);

typedef struct mupdf_attachment_s {
  mupdf_document_t* document;
  pdf_obj* file;
  int num;
  int gen;
} mupdf_attachment_t;

zathura_error_t
pdf_document_get_attachments(zathura_document_t* document, zathura_list_t** attachments)
{
  if (document == NULL || attachments == NULL) {
      return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;
  *attachments = NULL;

  mupdf_document_t* mupdf_document;
  if ((error = zathura_document_get_user_data(document, (void**) &mupdf_document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  pdf_obj* trailer = pdf_trailer(mupdf_document->ctx, (pdf_document*)
      mupdf_document->document);
  if (trailer == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  pdf_obj* root_dict = pdf_dict_get(mupdf_document->ctx, trailer, PDF_NAME_Root);
  if (root_dict == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  pdf_obj* names_dict = pdf_dict_get(mupdf_document->ctx, root_dict, PDF_NAME_Names);
  if (names_dict == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  pdf_obj* embedded_files_dict = pdf_dict_gets(mupdf_document->ctx, names_dict, "EmbeddedFiles");
  if (embedded_files_dict == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  pdf_obj* embedded_files_names = pdf_dict_get(mupdf_document->ctx, embedded_files_dict,
      PDF_NAME_Names);
  if (embedded_files_names == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  int length = pdf_array_len(mupdf_document->ctx, embedded_files_names);

  for (int i = 0; i < length; i += 2) {
    pdf_obj* file = pdf_array_get(mupdf_document->ctx, embedded_files_names, i+1);
    if (pdf_is_indirect(mupdf_document->ctx, file) == 0) {
      continue;
    }

    pdf_obj* res_file = pdf_resolve_indirect(mupdf_document->ctx, file);
    if (res_file == NULL) {
      continue;
    }

    zathura_attachment_t* attachment = parse_mupdf_attachment(mupdf_document, res_file);
    if (attachment != NULL) {
      *attachments = zathura_list_append(*attachments, attachment);
    }

    pdf_drop_obj(mupdf_document->ctx, res_file);
  }

  error = ZATHURA_ERROR_OK;

error_out:

  return error;
}

static zathura_attachment_t*
parse_mupdf_attachment(mupdf_document_t* mupdf_document, pdf_obj* file)
{
  if (pdf_is_dict(mupdf_document->ctx, file) == 0) {
    goto error_out;
  }

  pdf_obj* name = pdf_dict_gets(mupdf_document->ctx, file, "F");
  if (pdf_is_string(mupdf_document->ctx, name) == 0) {
    goto error_out;
  }

  pdf_obj* embedded_file_dict = pdf_dict_gets(mupdf_document->ctx, file, "EF");
  if (pdf_is_dict(mupdf_document->ctx, embedded_file_dict) == 0) {
    goto error_out;
  }

  pdf_obj* embedded_file_indirect = pdf_dict_gets(mupdf_document->ctx, embedded_file_dict, "F");
  if (pdf_is_indirect(mupdf_document->ctx, embedded_file_indirect) == 0) {
    goto error_out;
  }

  pdf_obj* embedded_file = pdf_resolve_indirect(mupdf_document->ctx, embedded_file_indirect);
  if (embedded_file == NULL) {
    goto error_out;
  }

  mupdf_attachment_t* mupdf_attachment = calloc(1, sizeof(mupdf_attachment_t));
  if (mupdf_attachment == NULL) {
    goto error_out;
  }

  mupdf_attachment->document = mupdf_document;
  mupdf_attachment->file     = embedded_file;
  mupdf_attachment->num      = pdf_to_num(mupdf_document->ctx, embedded_file_indirect);
  mupdf_attachment->gen      = pdf_to_gen(mupdf_document->ctx, embedded_file_indirect);

  zathura_attachment_t* attachment;
  if (zathura_attachment_new(&attachment) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  if (zathura_attachment_set_name(attachment, pdf_to_str_buf(mupdf_document->ctx, name))
      != ZATHURA_ERROR_OK) {
    goto error_free;
  }

  if (zathura_attachment_set_user_data(attachment, mupdf_attachment, free)
      != ZATHURA_ERROR_OK) {
    goto error_free;
  }

  if (zathura_attachment_set_save_function(attachment, pdf_attachment_save) !=
      ZATHURA_ERROR_OK) {
    goto error_free;
  }

  return attachment;

error_free:

    zathura_attachment_free(attachment);

error_out:

    return NULL;
}

static zathura_error_t
pdf_attachment_save(zathura_attachment_t* attachment, const char* path, void* user_data)
{
  if (attachment == NULL || path == NULL || strlen(path) == 0 || user_data == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  mupdf_attachment_t* mupdf_attachment = (mupdf_attachment_t*) user_data;
  mupdf_document_t* mupdf_document     = mupdf_attachment->document;
  pdf_obj* file                        = mupdf_attachment->file;

  if (pdf_is_dict(mupdf_document->ctx, file) == 0) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  if (pdf_is_stream(mupdf_document->ctx, (pdf_document*)
        mupdf_document->document, mupdf_attachment->num, mupdf_attachment->gen)
      == 0) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  fz_buffer* buffer = pdf_load_stream(mupdf_document->ctx, (pdf_document*)
      mupdf_document->document, mupdf_attachment->num, mupdf_attachment->gen);
  if (buffer == NULL) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  unsigned char* data;
  size_t data_length = fz_buffer_storage(mupdf_document->ctx, buffer, &data);

  FILE* f = fopen(path, "w+");
  if (f == NULL) {
    fz_drop_buffer(mupdf_document->ctx, buffer);
    return ZATHURA_ERROR_UNKNOWN;
  }

  if (fwrite(data, 1, data_length, f) < data_length) {
    fclose(f);
    fz_drop_buffer(mupdf_document->ctx, buffer);
    return ZATHURA_ERROR_UNKNOWN;
  }

  if (fclose(f) != 0) {
    fz_drop_buffer(mupdf_document->ctx, buffer);
    return ZATHURA_ERROR_UNKNOWN;
  }

  fz_drop_buffer(mupdf_document->ctx, buffer);

  return ZATHURA_ERROR_OK;
}
