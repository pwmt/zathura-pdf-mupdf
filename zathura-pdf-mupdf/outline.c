/* See LICENSE file for license and copyright information */

#include <stdio.h>

#include "plugin.h"
#include "internal.h"
#include "utils.h"

static void build_index(fz_outline* outline, zathura_node_t* root);

zathura_error_t
pdf_document_get_outline(zathura_document_t* document, zathura_node_t** outline)
{
  if (document == NULL || outline == NULL) {
      return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;

  mupdf_document_t* mupdf_document;
  if ((error = zathura_document_get_user_data(document, (void**) &mupdf_document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  /* get outline */
  fz_outline* mupdf_outline = fz_load_outline(mupdf_document->ctx, mupdf_document->document);
  if (mupdf_outline == NULL) {
    error = ZATHURA_ERROR_DOCUMENT_OUTLINE_DOES_NOT_EXIST;
    goto error_out;
  }

  /* generate index */
  zathura_outline_element_t* root;
  if ((error = zathura_outline_element_new(&root, NULL, NULL)) != ZATHURA_ERROR_OK) {
    goto error_free;
  }

  *outline = zathura_node_new(root);
  if (*outline == NULL) {
    error = ZATHURA_ERROR_OUT_OF_MEMORY;
    goto error_free;
  }

  build_index(mupdf_outline, *outline);

  /* free outline */
  fz_drop_outline(mupdf_document->ctx, mupdf_outline);

  return error;

error_free:

  zathura_outline_free(*outline);
  fz_drop_outline(mupdf_document->ctx, mupdf_outline);

error_out:

  return error;
}

static void
build_index(fz_outline* mupdf_outline, zathura_node_t* root)
{
  if (mupdf_outline == NULL || root == NULL) {
    return;
  }

  while (mupdf_outline != NULL) {
    zathura_action_t* action;
    if (mupdf_to_zathura_action(&(mupdf_outline->dest), &action) == false) {
      continue;
    }

    zathura_outline_element_t* outline_element;
    if (zathura_outline_element_new(&outline_element, mupdf_outline->title,
          action) != ZATHURA_ERROR_OK) {
      continue;
    }

    zathura_node_t* node = zathura_node_append_data(root, outline_element);

    if (mupdf_outline->down != NULL) {
      build_index(mupdf_outline->down, node);
    }

    mupdf_outline = mupdf_outline->next;
  }
}
