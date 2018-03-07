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

  fz_link* link = fz_load_links(mupdf_document->ctx, mupdf_page->page);
  for (; link != NULL; link = link->next) {
    /* extract position */
    zathura_rectangle_t position;
    position.x1 = link->rect.x0;
    position.x2 = link->rect.x1;
    position.y1 = link->rect.y0;
    position.y2 = link->rect.y1;

    zathura_link_type_t type     = ZATHURA_LINK_INVALID;
    zathura_link_target_t target = { 0 };

    if (fz_is_external_link(mupdf_document->ctx, link->uri) == 1) {
      if (strstr(link->uri, "file://") == link->uri) {
        type         = ZATHURA_LINK_GOTO_REMOTE;
        target.value = link->uri;
      } else {
        type         = ZATHURA_LINK_URI;
        target.value = link->uri;
      }
    } else {
      float x = 0;
      float y = 0;

      type                    = ZATHURA_LINK_GOTO_DEST;
      target.destination_type = ZATHURA_LINK_DESTINATION_XYZ;
      target.page_number      = fz_resolve_link(mupdf_document->ctx,
          mupdf_document->document, link->uri, &x, &y);
      target.left  = x;
      target.top   = y;
      target.zoom  = 0.0;
    }

    zathura_link_t* zathura_link = zathura_link_new(type, position, target);
    if (zathura_link != NULL) {
      girara_list_append(list, zathura_link);
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

