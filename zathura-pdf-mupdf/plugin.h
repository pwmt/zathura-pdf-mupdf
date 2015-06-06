/* See LICENSE file for license and copyright information */

#ifndef PDF_H
#define PDF_H

#if HAVE_CAIRO
#include <cairo.h>
#endif

#include <libzathura/plugin-api.h>

zathura_error_t pdf_document_open(zathura_document_t* document);
zathura_error_t pdf_document_free(zathura_document_t* document);
zathura_error_t pdf_document_save_as(zathura_document_t* document, const char* path);
zathura_error_t pdf_document_get_outline(zathura_document_t* document, zathura_node_t** outline);
zathura_error_t pdf_document_get_attachments(zathura_document_t* document, zathura_list_t** attachments);
zathura_error_t pdf_document_get_metadata(zathura_document_t* document, zathura_list_t** metadata);

zathura_error_t pdf_page_init(zathura_page_t* page);
zathura_error_t pdf_page_clear(zathura_page_t* page);
zathura_error_t pdf_page_search_text(zathura_page_t* page, const char* text, zathura_search_flag_t flags, zathura_list_t** results);
zathura_error_t pdf_page_get_text(zathura_page_t* page, char** text);
zathura_error_t pdf_page_get_selected_text(zathura_page_t* page, char** text, zathura_rectangle_t rectangle);
zathura_error_t pdf_page_get_links(zathura_page_t* page, zathura_list_t** links);
zathura_error_t pdf_page_get_form_fields(zathura_page_t* page, zathura_list_t** form_fields);
zathura_error_t pdf_page_get_images(zathura_page_t* page, zathura_list_t** images);
zathura_error_t pdf_page_get_annotations(zathura_page_t* page, zathura_list_t** annotations);
#ifdef HAVE_CAIRO
zathura_error_t pdf_page_render_cairo(zathura_page_t* page, cairo_t* cairo, double scale, int rotation, int flags);
#endif

zathura_error_t pdf_form_field_save(zathura_form_field_t* form_field);

#endif // PDF_H
