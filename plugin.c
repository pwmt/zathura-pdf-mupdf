/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include "plugin.h"

ZATHURA_PLUGIN_REGISTER_WITH_FUNCTIONS(
  "pdf-mupdf",
  VERSION_MAJOR, VERSION_MINOR, VERSION_REV,
  ZATHURA_PLUGIN_FUNCTIONS({
    .document_open            = (zathura_plugin_document_open_t) pdf_document_open,
    .document_free            = (zathura_plugin_document_free_t) pdf_document_free,
    .document_save_as         = (zathura_plugin_document_save_as_t) pdf_document_save_as,
    .document_index_generate  = (zathura_plugin_document_index_generate_t) pdf_document_index_generate,
    .document_get_information = (zathura_plugin_document_get_information_t) pdf_document_get_information,
    .page_init                = (zathura_plugin_page_init_t) pdf_page_init,
    .page_clear               = (zathura_plugin_page_clear_t) pdf_page_clear,
    .page_search_text         = (zathura_plugin_page_search_text_t) pdf_page_search_text,
    .page_links_get           = (zathura_plugin_page_links_get_t) pdf_page_links_get,
    .page_images_get          = (zathura_plugin_page_images_get_t) pdf_page_images_get,
    .page_get_text            = (zathura_plugin_page_get_text_t) pdf_page_get_text,
    .page_render              = (zathura_plugin_page_render_t) pdf_page_render,
    .page_render_cairo        = (zathura_plugin_page_render_cairo_t) pdf_page_render_cairo,
    .page_image_get_cairo     = (zathura_plugin_page_image_get_cairo_t) pdf_page_image_get_cairo
  }),
  ZATHURA_PLUGIN_MIMETYPES({
    "application/pdf",
    "application/oxps",
    "application/epub+zip"
  })
)
