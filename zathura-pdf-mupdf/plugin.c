/* SPDX-License-Identifier: Zlib */

#include "plugin.h"

#if defined(HAVE_PDF)
#define PDF_MIMETYPE "application/pdf",
#else
#define PDF_MIMETYPE
#endif

ZATHURA_PLUGIN_REGISTER_WITH_FUNCTIONS("pdf-mupdf", VERSION_MAJOR, VERSION_MINOR, VERSION_REV,
                                       ZATHURA_PLUGIN_FUNCTIONS({
                                           .document_open            = pdf_document_open,
                                           .document_free            = pdf_document_free,
                                           .document_save_as         = pdf_document_save_as,
                                           .document_index_generate  = pdf_document_index_generate,
                                           .document_get_information = pdf_document_get_information,
                                           .document_attachments_get = pdf_document_attachments_get,
                                           .document_attachment_save = pdf_document_attachment_save,
                                           .page_init                = pdf_page_init,
                                           .page_clear               = pdf_page_clear,
                                           .page_search_text         = pdf_page_search_text,
                                           .page_links_get           = pdf_page_links_get,
                                           .page_images_get          = pdf_page_images_get,
                                           .page_get_text            = pdf_page_get_text,
                                           .page_get_selection       = pdf_page_get_selection,
                                           .page_render_cairo        = pdf_page_render_cairo,
                                           .page_image_get_cairo     = pdf_page_image_get_cairo,
                                           .page_get_label           = pdf_page_get_label,
                                       }),
                                       ZATHURA_PLUGIN_MIMETYPES({
                                           PDF_MIMETYPE
                                           "application/oxps",
                                           "application/epub+zip",
                                           "application/x-fictionbook",
                                           "application/x-mobipocket-ebook",
                                           "text/xml",
                                           "image/svg+xml",
                                           "image/jpeg",
                                           "image/png",
                                           "image/bmp",
                                           "image/x-bmp",
                                           "image/tiff",
                                           "image/tiff-fx",
                                       }))
