/* See LICENSE file for license and copyright information */

#ifndef UTILS_H
#define UTILS_H

#include "plugin.h"
#include "internal.h"

void mupdf_page_extract_text(mupdf_document_t* mupdf_document,
    mupdf_page_t* mupdf_page);

bool mupdf_link_to_zathura_action(fz_context* ctx, const char* uri, zathura_action_t**
    action);

zathura_blend_mode_t mupdf_blend_mode_to_zathura_blend_mode(const char* blend_mode_str);

const char* zathura_blend_mode_to_mupdf_blend_mode(zathura_blend_mode_t blend_mode);

zathura_annotation_color_t mupdf_color_to_zathura_color(fz_context* ctx, pdf_obj* obj);

zathura_annotation_border_t mupdf_border_to_zathura_border(fz_context* ctx, pdf_obj* annotation);

#endif // UTILS_H
