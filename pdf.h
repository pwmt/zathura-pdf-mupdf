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
} mupdf_page_t;

/**
 * Open a pdf document
 *
 * @param document Zathura document
 * @return true if no error occured, otherwise false
 */
bool pdf_document_open(zathura_document_t* document);

/**
 * Closes and frees the internal document structure
 *
 * @param document Zathura document
 * @return true if no error occured, otherwise false
 */
bool pdf_document_free(zathura_document_t* document);

/**
 * Returns a reference to a page
 *
 * @param document Zathura document
 * @param page Page number
 * @return A page object or NULL if an error occured
 */
zathura_page_t* pdf_page_get(zathura_document_t* document, unsigned int page);

/**
 * Frees a pdf page
 *
 * @param page Page
 * @return true if no error occured, otherwise false
 */
bool pdf_page_free(zathura_page_t* page);

/**
 * Searches for a specific text on a page and returns a list of results
 *
 * @param page Page
 * @param text Search item
 * @return List of search results or NULL if an error occured
 */
girara_list_t* pdf_page_search_text(zathura_page_t* page, const char* text);

/**
 * Returns a list of internal/external links that are shown on the given page
 *
 * @param page Page
 * @return List of links or NULL if an error occured
 */
girara_list_t* pdf_page_links_get(zathura_page_t* page);

/**
 * Returns a list of form fields available on the given page
 *
 * @param page Page
 * @return List of form fields or NULL if an error occured
 */
girara_list_t* pdf_page_form_fields_get(zathura_page_t* page);

/**
 * Renders a page and returns a allocated image buffer which has to be freed
 * with zathura_image_buffer_free
 *
 * @param page Page
 * @return Image buffer or NULL if an error occured
 */
zathura_image_buffer_t* pdf_page_render(zathura_page_t* page);

#if HAVE_CAIRO
/**
 * Renders a page onto a cairo object
 *
 * @param page Page
 * @param cairo Cairo object
 * @return  true if no error occured, otherwise false
 */
bool pdf_page_render_cairo(zathura_page_t* page, cairo_t* cairo);
#endif

#endif // PDF_H
