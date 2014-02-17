/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include <girara/datastructures.h>

#include "plugin.h"

static void build_index(fz_outline* outline, girara_tree_node_t* root);

girara_tree_node_t*
pdf_document_index_generate(zathura_document_t* document, mupdf_document_t* mupdf_document, zathura_error_t* error)
{
  if (document == NULL || mupdf_document == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    return NULL;
  }

  /* get outline */
  fz_outline* outline = fz_load_outline(mupdf_document->document);
  if (outline == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_UNKNOWN;
    }
    return NULL;
  }

  /* generate index */
  girara_tree_node_t* root = girara_node_new(zathura_index_element_new("ROOT"));
  build_index(outline, root);

  /* free outline */
  fz_free_outline(mupdf_document->ctx, outline);

  return root;
}

static void
build_index(fz_outline* outline, girara_tree_node_t* root)
{
  if (outline == NULL || root == NULL) {
    return;
  }

  while (outline != NULL) {
    zathura_index_element_t* index_element = zathura_index_element_new(outline->title);
    zathura_link_target_t target;
    zathura_link_type_t type;
    zathura_rectangle_t rect;

    switch (outline->dest.kind) {
      case FZ_LINK_NONE:
        type = ZATHURA_LINK_NONE;
        break;
      case FZ_LINK_URI:
        type         = ZATHURA_LINK_URI;
        target.value = outline->dest.ld.uri.uri;
        break;
      case FZ_LINK_GOTO:
        type                    = ZATHURA_LINK_GOTO_DEST;
        target.page_number      = outline->dest.ld.gotor.page;
        target.destination_type = ZATHURA_LINK_DESTINATION_XYZ;
        target.left             = 0;
        target.top              = 0;
        target.scale            = 0.0;
        break;
      default:
        continue;
    }

    index_element->link = zathura_link_new(type, rect, target);
    if (index_element->link == NULL) {
      continue;
    }

    girara_tree_node_t* node = girara_node_append_data(root, index_element);

    if (outline->down != NULL) {
      build_index(outline->down, node);
    }

    outline = outline->next;
  }
}

