#include "plugin.h"
#include "utils.h"
#include <mupdf/pdf.h>

girara_list_t* pdf_document_attachments_get(zathura_document_t* document, void* data, zathura_error_t* error) {
  mupdf_document_t* mupdf_document = data;

  if (document == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  girara_list_t* list = NULL;

  /* Setup attachment list */
  list = girara_list_new();
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  girara_list_set_free_function(list, (girara_free_function_t)g_free);

  /* Extract attachments */
  g_mutex_lock(&mupdf_document->mutex);
  fz_try(mupdf_document->ctx) {
    pdf_document* pdf_doc = pdf_specifics(mupdf_document->ctx, mupdf_document->document);
    pdf_filespec_params fs_params;
    int n_objects  = pdf_xref_len(mupdf_document->ctx, pdf_doc);

    for (int i = 1; i < n_objects; ++i) {
      pdf_obj* obj = pdf_load_object(mupdf_document->ctx, pdf_doc, i);
      if (pdf_is_embedded_file(mupdf_document->ctx, obj)) {
        pdf_get_filespec_params(mupdf_document->ctx, obj, &fs_params);
        girara_list_append(list, g_strdup(fs_params.filename));
      }
    }
  }
  fz_catch(mupdf_document->ctx) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_UNKNOWN;
    }
    goto error_free;
  }
  g_mutex_unlock(&mupdf_document->mutex);

  return list;

error_free:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  if (list != NULL) {
    girara_list_free(list);
  }

error_ret:

  return NULL;
}

zathura_error_t pdf_document_attachment_save(zathura_document_t* document, void* UNUSED(data), const char* attachmentname, const char* file) {
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  g_mutex_lock(&mupdf_document->mutex);
  fz_try(mupdf_document->ctx) {
    pdf_document* pdf_doc = pdf_specifics(mupdf_document->ctx, mupdf_document->document);
    pdf_filespec_params fs_params;
    int n_objects  = pdf_xref_len(mupdf_document->ctx, pdf_doc);

    for (int i = 1; i < n_objects; ++i) {
      pdf_obj* obj = pdf_load_object(mupdf_document->ctx, pdf_doc, i);
      if (pdf_is_embedded_file(mupdf_document->ctx, obj)) {
        pdf_get_filespec_params(mupdf_document->ctx, obj, &fs_params);
        if (strcmp(fs_params.filename, attachmentname) != 0) {
          continue;
        }
        fz_save_buffer(mupdf_document->ctx, pdf_load_embedded_file_contents(mupdf_document->ctx, obj), file);
      }
    }
  }
  fz_catch(mupdf_document->ctx) {
    return ZATHURA_ERROR_UNKNOWN;
  }
  g_mutex_unlock(&mupdf_document->mutex);

  return ZATHURA_ERROR_OK;
}
