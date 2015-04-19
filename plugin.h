/* See LICENSE file for license and copyright information */

#ifndef PDF_H
#define PDF_H

#include <stdbool.h>
#include <zathura/plugin-api.h>
#include <mupdf/fitz.h>

#if HAVE_CAIRO
#include <cairo.h>
#endif

typedef struct mupdf_document_s
{
  fz_context* ctx; /**< Context */
  fz_document* document; /**< mupdf document */
} mupdf_document_t;

typedef struct mupdf_page_s
{
  fz_page* page; /**< Reference to the mupdf page */
  fz_context* ctx; /**< Context */
  fz_text_sheet* sheet; /**< Text sheet */
  fz_text_page* text; /**< Page text */
  fz_rect bbox; /**< Bbox */
  bool extracted_text; /**< If text has already been extracted */
} mupdf_page_t;

/**
 * Open a pdf document
 *
 * @param document Zathura document
 * @return true if no error occured, otherwise false
 */
zathura_error_t pdf_document_open(zathura_document_t* document);

/**
 * Closes and frees the internal document structure
 *
 * @param document Zathura document
 * @return true if no error occured, otherwise false
 */
zathura_error_t pdf_document_free(zathura_document_t* document, mupdf_document_t* mupdf_document);

/**
 * Saves the document to the given path
 *
 * @param document Zathura document
 * @param path File path
 * @return ZATHURA_ERROR_OK when no error occured, otherwise see
 *    zathura_error_t
 */
zathura_error_t pdf_document_save_as(zathura_document_t* document,
    mupdf_document_t* mupdf_document, const char* path);

/**
 * Generates the index of the document
 *
 * @param document Zathura document
 * @param error Set to an error value (see zathura_error_t) if an
 *   error occured
 * @return Tree node object or NULL if an error occurred (e.g.: the document has
 *   no index)
 */
girara_tree_node_t* pdf_document_index_generate(zathura_document_t* document,
    mupdf_document_t* mupdf_document, zathura_error_t* error);

/**
 * Returns a reference to a page
 *
 * @param page Page object
 * @return A page object or NULL if an error occured
 */
zathura_error_t pdf_page_init(zathura_page_t* page);

/**
 * Frees a pdf page
 *
 * @param page Page
 * @return true if no error occured, otherwise false
 */
zathura_error_t pdf_page_clear(zathura_page_t* page, mupdf_page_t* mupdf_page);

/**
 * Searches for a specific text on a page and returns a list of results
 *
 * @param page Page
 * @param text Search item
 * @param error Set to an error value (see zathura_error_t) if an
 *   error occured
 * @return List of search results or NULL if an error occured
 */
girara_list_t* pdf_page_search_text(zathura_page_t* page, mupdf_page_t* mupdf_page, const char* text, zathura_error_t* error);

/**
 * Returns a list of internal/external links that are shown on the given page
 *
 * @param page Page
 * @param error Set to an error value (see zathura_error_t) if an
 *   error occured
 * @return List of links or NULL if an error occured
 */
girara_list_t* pdf_page_links_get(zathura_page_t* page, mupdf_page_t* mupdf_page, zathura_error_t* error);

/**
 * Returns a list of images included on the zathura page
 *
 * @param page The page
 * @param error Set to an error value (see zathura_error_t) if an
 *   error occured
 * @return List of images
 */
girara_list_t* pdf_page_images_get(zathura_page_t* page, mupdf_page_t* mupdf_page, zathura_error_t* error);

#if HAVE_CAIRO
/**
 * Gets the content of the image in a cairo surface
 *
 * @param page Page
 * @param image Image identifier
 * @param error Set to an error value (see \ref zathura_error_t) if an
 *   error occured
 * @return The cairo image surface or NULL if an error occured
 */
cairo_surface_t* pdf_page_image_get_cairo(zathura_page_t* page, mupdf_page_t*
    mupdf_page, zathura_image_t* image, zathura_error_t* error);
#endif

/**
 * Get text for selection
 * @param page Page
 * @param rectangle Selection
 * @error Set to an error value (see \ref zathura_error_t) if an error
 * occured
 * @return The selected text (needs to be deallocated with g_free)
 */
char* pdf_page_get_text(zathura_page_t* page, mupdf_page_t* mupdf_page, zathura_rectangle_t rectangle, zathura_error_t* error);

/**
 * Returns a list of document information entries of the document
 *
 * @param document Zathura document
 * @param error Set to an error value (see zathura_error_t) if an
 *   error occured
 * @return List of information entries or NULL if an error occurred
 */
girara_list_t* pdf_document_get_information(zathura_document_t* document,
    mupdf_document_t* mupdf_document, zathura_error_t* error);

/**
 * Renders a page and returns a allocated image buffer which has to be freed
 * with zathura_image_buffer_free
 *
 * @param page Page
 * @param error Set to an error value (see zathura_error_t) if an
 *   error occured
 * @return Image buffer or NULL if an error occured
 */
zathura_image_buffer_t* pdf_page_render(zathura_page_t* page, mupdf_page_t* mupdf_page, zathura_error_t* error);

#if HAVE_CAIRO
/**
 * Renders a page onto a cairo object
 *
 * @param page Page
 * @param cairo Cairo object
 * @return  true if no error occured, otherwise false
 */
zathura_error_t pdf_page_render_cairo(zathura_page_t* page, mupdf_page_t* mupdf_page, cairo_t* cairo, bool printing);
#endif

#endif // PDF_H
