/* See LICENSE file for license and copyright information */

#include <string.h>
#include <stdio.h>

#include "plugin.h"

zathura_error_t
pdf_document_get_metadata(zathura_document_t* document,
    zathura_list_t** metadata)
{
  if (document == NULL || metadata == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_PLUGIN_NOT_IMPLEMENTED;

  return error;
}
