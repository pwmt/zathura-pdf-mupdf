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
  fz_outline* outline = fz_load_outline(mupdf_document->ctx, mupdf_document->document);
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
  fz_drop_outline(mupdf_document->ctx, outline);

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
    zathura_link_target_t target           = { ZATHURA_LINK_DESTINATION_UNKNOWN, NULL, 0, -1, -1, -1, -1, 0 };
    zathura_link_type_t type               = ZATHURA_LINK_INVALID;
    zathura_rectangle_t rect               = { .x1 = 0, .y1 = 0, .x2 = 0, .y2 = 0 };

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
        {
          int gflags = outline->dest.ld.gotor.flags;
          if (gflags & fz_link_flag_l_valid) {
            target.left = outline->dest.ld.gotor.lt.x;
          }
          if (gflags & fz_link_flag_t_valid) {
            target.top = outline->dest.ld.gotor.lt.y;
          }
          /* if (gflags & fz_link_flag_r_is_zoom) { */
          /*   target.scale = outline->dest.ld.gotor.rb.x; */
          /* } */
        }
        break;
      case FZ_LINK_LAUNCH:
        type = ZATHURA_LINK_LAUNCH;
        target.value = outline->dest.ld.launch.file_spec;
        break;
      case FZ_LINK_NAMED:
        type = ZATHURA_LINK_NAMED;
        target.value = outline->dest.ld.named.named;
        break;
      case FZ_LINK_GOTOR:
        type = ZATHURA_LINK_GOTO_REMOTE;
        target.value = outline->dest.ld.gotor.file_spec;
        break;
      default:
        outline = outline->next; // TODO: Don't skip unknown type
        continue;
    }

    index_element->link = zathura_link_new(type, rect, target);
    if (index_element->link == NULL) {
      outline = outline->next;
      continue;
    }

    girara_tree_node_t* node = girara_node_append_data(root, index_element);

    if (outline->down != NULL) {
      build_index(outline->down, node);
    }

    outline = outline->next;
  }
}

