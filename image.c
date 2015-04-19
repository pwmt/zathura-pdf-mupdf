/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include <glib.h>
#include <girara/datastructures.h>
#include <mupdf/pdf.h>

#include "plugin.h"

static void pdf_zathura_image_free(zathura_image_t* image);
static void get_images(zathura_page_t* page, pdf_obj* dict, girara_list_t* list);
static void get_resources(zathura_page_t* page, pdf_obj* resource, girara_list_t* list);

girara_list_t*
pdf_page_images_get(zathura_page_t* page, mupdf_page_t* mupdf_page, zathura_error_t* error)
{
  if (page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  girara_list_t* list = NULL;
  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL) {
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  pdf_obj* page_object = pdf_load_object(mupdf_document->ctx, (pdf_document*) mupdf_document->document, zathura_page_get_index(page), 0);
  if (page_object == NULL) {
    goto error_free;
  }

  pdf_obj* resource = pdf_dict_gets(mupdf_document->ctx, page_object, "Resources");
  if (resource == NULL) {
    goto error_free;
  }

  list = girara_list_new();
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  girara_list_set_free_function(list, (girara_free_function_t) pdf_zathura_image_free);

  get_resources(page, resource, list);

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


static void
pdf_zathura_image_free(zathura_image_t* image)
{
  if (image == NULL) {
    return;
  }

  g_free(image);
}

static void
get_images(zathura_page_t* page, pdf_obj* dict, girara_list_t* list)
{
  if (dict == NULL || list == NULL) {
    return;
  }

  if (page == NULL) {
    return;
  }

  zathura_document_t* document = zathura_page_get_document(page);

  if (document == NULL) {
    return;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  for (int i = 0; i < pdf_dict_len(mupdf_document->ctx, dict); i++) {
    pdf_obj* image_dict = pdf_dict_get_val(mupdf_document->ctx, dict, i);
    if (pdf_is_dict(mupdf_document->ctx, image_dict) == 0) {
      continue;
    }

    pdf_obj* type = pdf_dict_gets(mupdf_document->ctx, image_dict, "Subtype");
    if (strcmp(pdf_to_name(mupdf_document->ctx, type), "Image") != 0) {
      continue;
    }

    bool duplicate = false;
    GIRARA_LIST_FOREACH(list, zathura_image_t*, iter, image)
      if (image->data == image_dict) {
        duplicate = true;
        break;
      }
    GIRARA_LIST_FOREACH_END(list, zathura_image_t*, iter, image);

    if (duplicate == true) {
      continue;
    }

    pdf_obj* width  = pdf_dict_gets(mupdf_document->ctx, image_dict, "Width");
    pdf_obj* height = pdf_dict_gets(mupdf_document->ctx, image_dict, "Height");

    zathura_image_t* zathura_image = g_malloc(sizeof(zathura_image_t));

    fprintf(stderr, "image\n");

    // FIXME: Get correct image coordinates
    zathura_image->data        = image_dict;
    zathura_image->position.x1 = 0;
    zathura_image->position.x2 = pdf_to_int(mupdf_document->ctx, width);
    zathura_image->position.y1 = 0;
    zathura_image->position.y2 = pdf_to_int(mupdf_document->ctx, height);

    girara_list_append(list, zathura_image);
  }
}

static void
get_resources(zathura_page_t* page, pdf_obj* resource, girara_list_t* list)
{
  if (resource == NULL || list == NULL) {
    return;
  }

  if (page == NULL) {
    return;
  }

  zathura_document_t* document = zathura_page_get_document(page);

  if (document == NULL) {
    return;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  pdf_obj* x_object = pdf_dict_gets(mupdf_document->ctx, resource, "XObject");
  if (x_object == NULL) {
    return;
  }

  get_images(page, x_object, list);

  for (int i = 0; i < pdf_dict_len(mupdf_document->ctx, x_object); i++) {
    pdf_obj* obj = pdf_dict_get_val(mupdf_document->ctx, x_object, i);
    pdf_obj* subsrc = pdf_dict_gets(mupdf_document->ctx, obj, "Resources");
    if (subsrc != NULL && pdf_objcmp(mupdf_document->ctx, resource, subsrc)) {
      get_resources(page, subsrc, list);
    }
  }
}

