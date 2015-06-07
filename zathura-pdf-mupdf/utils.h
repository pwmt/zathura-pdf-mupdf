/* See LICENSE file for license and copyright information */

#ifndef UTILS_H
#define UTILS_H

#include "plugin.h"
#include "internal.h"

void mupdf_page_extract_text(mupdf_document_t* mupdf_document,
    mupdf_page_t* mupdf_page);

bool mupdf_to_zathura_action(fz_link_dest* link, zathura_action_t**
    action);

#endif // UTILS_H
