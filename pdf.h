/* See LICENSE file for license and copyright information */

#ifndef PDF_H
#define PDF_H

#include <stdbool.h>
#include <zathura/document.h>
#include <fitz.h>
#include <mupdf.h>

#if HAVE_CAIRO
#include <cairo.h>
#endif

typedef struct pdf_document_s
{
  fz_glyph_cache *glyph_cache; /* Glyph cache */
  pdf_xref *document; /* mupdf document */
} pdf_document_t;

typedef struct mupdf_page_s
{
  pdf_page* page; /* Reference to the mupdf page */
  fz_text_span* text; /* Page text */
  bool extracted_text; /* Text has been extracted */
} mupdf_page_t;

/**
 * Open a pdf document
 *
 * @param document Zathura document
 * @return true if no error occured, otherwise false
 */
zathura_plugin_error_t pdf_document_open(zathura_document_t* document);

/**
 * Closes and frees the internal document structure
 *
 * @param document Zathura document
 * @return true if no error occured, otherwise false
 */
zathura_plugin_error_t pdf_document_free(zathura_document_t* document);

/**
 * Returns a reference to a page
 *
 * @param document Zathura document
 * @param page Page number
 * @param error Set to an error value (see zathura_plugin_error_t) if an
 *   error occured
 * @return A page object or NULL if an error occured
 */
zathura_page_t* pdf_page_get(zathura_document_t* document, unsigned int page, zathura_plugin_error_t* error);

/**
 * Frees a pdf page
 *
 * @param page Page
 * @return true if no error occured, otherwise false
 */
zathura_plugin_error_t pdf_page_free(zathura_page_t* page);

/**
 * Searches for a specific text on a page and returns a list of results
 *
 * @param page Page
 * @param text Search item
 * @param error Set to an error value (see zathura_plugin_error_t) if an
 *   error occured
 * @return List of search results or NULL if an error occured
 */
girara_list_t* pdf_page_search_text(zathura_page_t* page, const char* text, zathura_plugin_error_t* error);

/**
 * Returns a list of internal/external links that are shown on the given page
 *
 * @param page Page
 * @param error Set to an error value (see zathura_plugin_error_t) if an
 *   error occured
 * @return List of links or NULL if an error occured
 */
girara_list_t* pdf_page_links_get(zathura_page_t* page, zathura_plugin_error_t* error);

/**
 * Returns a list of images included on the zathura page
 *
 * @param page The page
 * @param error Set to an error value (see zathura_plugin_error_t) if an
 *   error occured
 * @return List of images
 */
girara_list_t* pdf_page_images_get(zathura_page_t* page, zathura_plugin_error_t* error);

/**
 * Get text for selection
 * @param page Page
 * @param rectangle Selection
 * @error Set to an error value (see \ref zathura_plugin_error_t) if an error
 * occured
 * @return The selected text (needs to be deallocated with g_free)
 */
char* pdf_page_get_text(zathura_page_t* page, zathura_rectangle_t rectangle, zathura_plugin_error_t* error);

/**
 * Renders a page and returns a allocated image buffer which has to be freed
 * with zathura_image_buffer_free
 *
 * @param page Page
 * @param error Set to an error value (see zathura_plugin_error_t) if an
 *   error occured
 * @return Image buffer or NULL if an error occured
 */
zathura_image_buffer_t* pdf_page_render(zathura_page_t* page, zathura_plugin_error_t* error);

#if HAVE_CAIRO
/**
 * Renders a page onto a cairo object
 *
 * @param page Page
 * @param cairo Cairo object
 * @return  true if no error occured, otherwise false
 */
zathura_plugin_error_t pdf_page_render_cairo(zathura_page_t* page, cairo_t* cairo, bool printing);
#endif

#endif // PDF_H
