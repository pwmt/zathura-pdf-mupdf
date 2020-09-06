/* SPDX-License-Identifier: Zlib */

#include <girara/datastructures.h>

#include "plugin.h"

static void build_index(fz_context* ctx, fz_document* document, fz_outline*
    outline, girara_tree_node_t* root);

girara_tree_node_t*
pdf_document_index_generate(zathura_document_t* document, void* data, zathura_error_t* error)
{
  mupdf_document_t* mupdf_document = data;

  if (document == NULL || mupdf_document == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    return NULL;
  }

  /* get outline */
  fz_outline* outline = fz_load_outline(mupdf_document->ctx, mupdf_document->document);
  if (outline == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_UNKNOWN;
    }
    return NULL;
  }

  /* generate index */
  girara_tree_node_t* root = girara_node_new(zathura_index_element_new("ROOT"));
  build_index(mupdf_document->ctx, mupdf_document->document, outline, root);

  /* free outline */
  fz_drop_outline(mupdf_document->ctx, outline);

  return root;
}

static void
build_index(fz_context* ctx, fz_document* document, fz_outline* outline, girara_tree_node_t* root)
{
  if (outline == NULL || root == NULL) {
    return;
  }

  while (outline != NULL) {
    zathura_index_element_t* index_element = zathura_index_element_new(outline->title);
    zathura_link_target_t target           = { ZATHURA_LINK_DESTINATION_UNKNOWN, NULL, 0, -1, -1, -1, -1, 0 };
    zathura_link_type_t type               = ZATHURA_LINK_INVALID;
    zathura_rectangle_t rect               = { .x1 = 0, .y1 = 0, .x2 = 0, .y2 = 0 };

    if (outline->uri == NULL) {
      type = ZATHURA_LINK_NONE;
    } else if (fz_is_external_link(ctx, outline->uri) == 1) {
      if (strstr(outline->uri, "file://") == outline->uri) {
        type         = ZATHURA_LINK_GOTO_REMOTE;
        target.value = outline->uri;
      } else {
        type         = ZATHURA_LINK_URI;
        target.value = outline->uri;
      }
    } else {
      float x = 0;
      float y = 0;

      fz_location location = fz_resolve_link(ctx, document, outline->uri, &x, &y);

      type                    = ZATHURA_LINK_GOTO_DEST;
      target.destination_type = ZATHURA_LINK_DESTINATION_XYZ;
      target.page_number      = location.page;
      target.left  = x;
      target.top   = y;
      target.zoom  = 0.0;
    }

    index_element->link = zathura_link_new(type, rect, target);
    if (index_element->link == NULL) {
      outline = outline->next;
      continue;
    }

    girara_tree_node_t* node = girara_node_append_data(root, index_element);

    if (outline->down != NULL) {
      build_index(ctx, document, outline->down, node);
    }

    outline = outline->next;
  }
}

