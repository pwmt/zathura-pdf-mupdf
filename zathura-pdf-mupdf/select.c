/* See LICENSE file for license and copyright information */

#include <stdio.h>

#include "plugin.h"
#include "internal.h"

zathura_error_t
pdf_page_get_text(zathura_page_t* page, char** text)
{
  if (page == NULL || text == NULL) {
      return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_PLUGIN_NOT_IMPLEMENTED;

  return error;
}

zathura_error_t
pdf_page_get_selected_text(zathura_page_t* page, char** text, zathura_rectangle_t rectangle)
{
  if (page == NULL || text == NULL) {
      return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_PLUGIN_NOT_IMPLEMENTED;

  return error;
}
