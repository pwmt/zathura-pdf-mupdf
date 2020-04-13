/* See LICENSE file for license and copyright information */

#include <stdlib.h>

#include "plugin.h"
#include "internal.h"
#include "utils.h"

zathura_error_t
pdf_page_get_links(zathura_page_t* page, zathura_list_t** links)
{
  if (page == NULL || links == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;
  *links = NULL;

  zathura_document_t* document;
  if ((error = zathura_page_get_document(page, &document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_document_t* mupdf_document;
  if ((error = zathura_document_get_user_data(document, (void**) &mupdf_document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_page_t* mupdf_page;
  if ((error = zathura_page_get_user_data(page, (void**) &mupdf_page)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  fz_link* link = fz_load_links(mupdf_document->ctx, mupdf_page->page);
  for (; link != NULL; link = link->next) {
    /* extract position */
    zathura_rectangle_t position;
    position.p1.x = link->rect.x0;
    position.p2.x = link->rect.x1;
    position.p1.y = link->rect.y0;
    position.p2.y = link->rect.y1;

    zathura_action_t* action;
    if (mupdf_to_zathura_action(&(link->uri), &action) == false) {
      continue;
    }

    zathura_link_mapping_t* link_mapping = calloc(1, sizeof(zathura_link_mapping_t));
    if (link_mapping == NULL) {
      error = ZATHURA_ERROR_OUT_OF_MEMORY;
      goto error_free;
    }

    link_mapping->position = position;
    link_mapping->action = action;

    *links = zathura_list_append(*links, link_mapping);
  }

  return error;

error_free:

  if (*links != NULL) {
    zathura_list_free_full(*links, free);
    *links = NULL;
  }

error_out:

  return error;
}
