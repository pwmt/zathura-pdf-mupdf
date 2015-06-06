/* See LICENSE file for license and copyright information */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "plugin.h"
#include "internal.h"

zathura_error_t pdf_page_search_text(zathura_page_t* page, const char* text,
    zathura_search_flag_t flags, zathura_list_t** results)
{
  if (page == NULL || text == NULL || strlen(text) == 0 || results == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_PLUGIN_NOT_IMPLEMENTED;

  return error;
}
