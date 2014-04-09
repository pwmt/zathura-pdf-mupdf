/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include <glib.h>

#include "plugin.h"

girara_list_t*
pdf_page_links_get(zathura_page_t* page, mupdf_page_t* mupdf_page, zathura_error_t* error)
{
  if (page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL || mupdf_page == NULL || mupdf_page->page == NULL) {
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);;

  girara_list_t* list = girara_list_new2((girara_free_function_t) zathura_link_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  fz_link* link = fz_load_links(mupdf_document->document, mupdf_page->page);
  for (; link != NULL; link = link->next) {
    /* extract position */
    zathura_rectangle_t position;
    position.x1 = link->rect.x0;
    position.x2 = link->rect.x1;
    position.y1 = link->rect.y0;
    position.y2 = link->rect.y1;

    zathura_link_type_t type     = ZATHURA_LINK_INVALID;
    zathura_link_target_t target = { 0 };

    char* buffer = NULL;
    switch (link->dest.kind) {
      case FZ_LINK_NONE:
        type = ZATHURA_LINK_NONE;
        break;
      case FZ_LINK_URI:
        type         = ZATHURA_LINK_URI;
        target.value = link->dest.ld.uri.uri;
        break;
      case FZ_LINK_GOTO:
        type                    = ZATHURA_LINK_GOTO_DEST;
        target.page_number      = link->dest.ld.gotor.page;
        target.destination_type = ZATHURA_LINK_DESTINATION_XYZ;
        target.left             = 0;
        target.top              = 0;
        target.scale            = 0.0;
        {
          int gflags = link->dest.ld.gotor.flags;
          if (gflags & fz_link_flag_l_valid) {
            target.left = link->dest.ld.gotor.lt.x;
          }
          if (gflags & fz_link_flag_t_valid) {
            target.top = link->dest.ld.gotor.lt.y;
          }
          /* if (gflags & fz_link_flag_r_is_zoom) { */
          /*   target.scale = link->dest.ld.gotor.rb.x; */
          /* } */
        }
        break;
      default:
        continue;
    }

    zathura_link_t* zathura_link = zathura_link_new(type, position, target);
    if (zathura_link != NULL) {
      girara_list_append(list, zathura_link);
    }

    if (buffer != NULL) {
      g_free(buffer);
    }
  }

  return list;

error_free:

  if (list != NULL) {
    girara_list_free(list);
  }

error_ret:

  return NULL;
}

