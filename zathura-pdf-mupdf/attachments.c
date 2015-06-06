/* See LICENSE file for license and copyright information */

#include <stdio.h>

#include "plugin.h"
#include "macros.h"

zathura_error_t
pdf_document_get_attachments(zathura_document_t* document, zathura_list_t** attachments)
{
  if (document == NULL || attachments == NULL) {
      return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_PLUGIN_NOT_IMPLEMENTED;

  return error;
}
