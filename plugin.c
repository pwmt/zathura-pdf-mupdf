/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include "plugin.h"

void
register_functions(zathura_plugin_functions_t* functions)
{
  functions->document_open            = (zathura_plugin_document_open_t) pdf_document_open;
  functions->document_free            = (zathura_plugin_document_free_t) pdf_document_free;
  functions->document_save_as         = (zathura_plugin_document_save_as_t) pdf_document_save_as;
  functions->document_index_generate  = (zathura_plugin_document_index_generate_t) pdf_document_index_generate;
  functions->page_init                = (zathura_plugin_page_init_t) pdf_page_init;
  functions->page_clear               = (zathura_plugin_page_clear_t) pdf_page_clear;
  functions->page_search_text         = (zathura_plugin_page_search_text_t) pdf_page_search_text;
  functions->page_links_get           = (zathura_plugin_page_links_get_t) pdf_page_links_get;
#if 0
  functions->document_get_information = (zathura_plugin_document_get_information_t) pdf_document_get_information;
  functions->page_images_get          = (zathura_plugin_page_images_get_t) pdf_page_images_get;
#endif
  functions->page_get_text            = (zathura_plugin_page_get_text_t) pdf_page_get_text;
  functions->page_render              = (zathura_plugin_page_render_t) pdf_page_render;
#if HAVE_CAIRO
  functions->page_render_cairo        = (zathura_plugin_page_render_cairo_t) pdf_page_render_cairo;
#endif
}

ZATHURA_PLUGIN_REGISTER(
  "pdf-mupdf",
  VERSION_MAJOR, VERSION_MINOR, VERSION_REV,
  register_functions,
  ZATHURA_PLUGIN_MIMETYPES({
    "application/pdf",
    "application/oxps"
  })
)
