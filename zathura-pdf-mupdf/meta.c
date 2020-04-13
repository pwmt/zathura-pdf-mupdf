/* See LICENSE file for license and copyright information */

#include <string.h>
#include <stdio.h>

#include "plugin.h"
#include "internal.h"
#include "macros.h"

zathura_error_t
pdf_document_get_metadata(zathura_document_t* document,
    zathura_list_t** metadata)
{
  if (document == NULL || metadata == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;
  *metadata = NULL;

  mupdf_document_t* mupdf_document;
  if ((error = zathura_document_get_user_data(document, (void**) &mupdf_document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  fz_try (mupdf_document->ctx) {
    pdf_obj* trailer = pdf_trailer(mupdf_document->ctx, (pdf_document*) mupdf_document->document);
    pdf_obj* info_dict = pdf_dict_get(mupdf_document->ctx, trailer, PDF_NAME(Info));

    /* get string values */
    typedef struct info_value_s {
      const char* property;
      zathura_document_meta_type_t type;
    } info_value_t;

    static const info_value_t string_values[] = {
      { "Title",    ZATHURA_DOCUMENT_META_TITLE },
      { "Author",   ZATHURA_DOCUMENT_META_AUTHOR },
      { "Subject",  ZATHURA_DOCUMENT_META_SUBJECT },
      { "Keywords", ZATHURA_DOCUMENT_META_KEYWORDS },
      { "Creator",  ZATHURA_DOCUMENT_META_CREATOR },
      { "Producer", ZATHURA_DOCUMENT_META_PRODUCER }
    };

    for (unsigned int i = 0; i < LENGTH(string_values); i++) {
      pdf_obj* value = pdf_dict_gets(mupdf_document->ctx, info_dict, string_values[i].property);
      if (value == NULL) {
        continue;
      }

      char* string_value = pdf_to_str_buf(mupdf_document->ctx, value);
      if (string_value == NULL || strlen(string_value) == 0) {
        continue;
      }

      zathura_document_meta_entry_t* entry;
      if (zathura_document_meta_entry_new(&entry, string_values[i].type, string_value) != ZATHURA_ERROR_OK) {
        g_free(string_value);
        continue;
      }

      *metadata = zathura_list_append(*metadata, entry);
    }

    static const info_value_t time_values[] = {
      { "CreationDate", ZATHURA_DOCUMENT_META_CREATION_DATE },
      { "ModDate",      ZATHURA_DOCUMENT_META_MODIFICATION_DATE }
    };

    for (unsigned int i = 0; i < LENGTH(time_values); i++) {
      pdf_obj* value = pdf_dict_gets(mupdf_document->ctx, info_dict, time_values[i].property);
      if (value == NULL) {
        continue;
      }

      char* string_value = pdf_to_str_buf(mupdf_document->ctx, value);
      if (string_value == NULL || strlen(string_value) == 0) {
        continue;
      }

      zathura_document_meta_entry_t* entry;
      if (zathura_document_meta_entry_new(&entry, time_values[i].type, string_value) != ZATHURA_ERROR_OK) {
        g_free(string_value);
        continue;
      }

      *metadata = zathura_list_append(*metadata, entry);
    }
  } fz_catch (mupdf_document->ctx) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  return error;

error_free:

  zathura_list_free_full(*metadata, zathura_document_meta_entry_free);

error_out:

  return error;
}
