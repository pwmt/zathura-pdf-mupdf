/* See LICENSE file for license and copyright information */

#include "plugin.h"

void
register_functions(zathura_plugin_functions_t* functions)
{
  functions->document_open            = (zathura_plugin_document_open_t) pdf_document_open;
  functions->document_free            = (zathura_plugin_document_free_t) pdf_document_free;
  functions->document_save_as         = (zathura_plugin_document_save_as_t) pdf_document_save_as;
  functions->document_get_outline     = (zathura_plugin_document_get_outline_t) pdf_document_get_outline;
  functions->document_get_attachments = (zathura_plugin_document_get_attachments_t) pdf_document_get_attachments;
  functions->document_get_metadata    = (zathura_plugin_document_get_metadata_t) pdf_document_get_metadata;

  functions->page_init                = (zathura_plugin_page_init_t) pdf_page_init;
  functions->page_clear               = (zathura_plugin_page_clear_t) pdf_page_clear;
  functions->page_search_text         = (zathura_plugin_page_search_text_t) pdf_page_search_text;
  functions->page_get_text            = (zathura_plugin_page_get_text_t) pdf_page_get_text;
  functions->page_get_selected_text   = (zathura_plugin_page_get_selected_text_t) pdf_page_get_selected_text;
  functions->page_get_links           = (zathura_plugin_page_get_links_t) pdf_page_get_links;
  functions->page_get_images          = (zathura_plugin_page_get_images_t) pdf_page_get_images;
  functions->page_get_form_fields     = (zathura_plugin_page_get_form_fields_t) pdf_page_get_form_fields;
  functions->page_get_annotations     = (zathura_plugin_page_get_annotations_t) pdf_page_get_annotations;
#if HAVE_CAIRO
  functions->page_render_cairo        = (zathura_plugin_page_render_cairo_t) pdf_page_render_cairo;
#endif

  functions->form_field_save          = (zathura_plugin_form_field_save_t) pdf_form_field_save;

/* #if HAVE_CAIRO */
  functions->form_field_render_cairo  = (zathura_plugin_form_field_render_cairo_t) pdf_form_field_render_cairo;
/* #endif */

#if HAVE_CAIRO
  functions->annotation_render_cairo  = (zathura_plugin_annotation_render_cairo_t) pdf_annotation_render_cairo;
#endif

}

/* LCOV_EXCL_START */
ZATHURA_PLUGIN_REGISTER(
  "pdf-poppler",
  VERSION_MAJOR, VERSION_MINOR, VERSION_REV,
  register_functions,
  ZATHURA_PLUGIN_MIMETYPES({
    "application/pdf",
    "application/oxps",
    "application/epub+zip"
  })
)
