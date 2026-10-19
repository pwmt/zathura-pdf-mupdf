/* SPDX-License-Identifier: Zlib */

#define MAX_QUADS 1000

#include "plugin.h"
#include "utils.h"

char* pdf_page_get_text(zathura_page_t* page, void* data, zathura_rectangle_t rectangle, zathura_error_t* error) {
  mupdf_page_t* mupdf_page = data;

  if (page == NULL || mupdf_page == NULL || mupdf_page->text == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);
  g_mutex_lock(&mupdf_document->mutex);

  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document, mupdf_page);
  }

  fz_point a = {rectangle.x1, rectangle.y1};
  fz_point b = {rectangle.x2, rectangle.y2};

  char* ret = NULL;
#ifdef _WIN32
  ret = fz_copy_selection(mupdf_page->ctx, mupdf_page->text, a, b, 1);
#else
  ret = fz_copy_selection(mupdf_page->ctx, mupdf_page->text, a, b, 0);
#endif
  g_mutex_unlock(&mupdf_document->mutex);
  return ret;

error_ret:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  return NULL;
}

girara_list_t* pdf_page_get_selection(zathura_page_t* page, void* data, zathura_rectangle_t rectangle,
                                      zathura_error_t* error) {

  mupdf_page_t* mupdf_page = data;

  if (page == NULL || mupdf_page == NULL || mupdf_page->text == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);
  g_mutex_lock(&mupdf_document->mutex);

  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document, mupdf_page);
  }

  fz_point a = {rectangle.x1, rectangle.y1};
  fz_point b = {rectangle.x2, rectangle.y2};

  girara_list_t* list = girara_list_new_with_free(g_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  fz_quad* hits   = fz_malloc_array(mupdf_page->ctx, MAX_QUADS, fz_quad);
  int num_results = fz_highlight_selection(mupdf_page->ctx, mupdf_page->text, a, b, hits, MAX_QUADS);

  fz_rect r;
  for (int i = 0; i < num_results; i++) {
    zathura_rectangle_t* inner_rectangle = g_malloc0(sizeof(zathura_rectangle_t));

    r                   = fz_rect_from_quad(hits[i]);
    inner_rectangle->x1 = r.x0;
    inner_rectangle->x2 = r.x1;
    inner_rectangle->y1 = r.y0;
    inner_rectangle->y2 = r.y1;

    girara_list_append(list, inner_rectangle);
  }

  fz_free(mupdf_page->ctx, hits);
  g_mutex_unlock(&mupdf_document->mutex);

  return list;

error_free:
  g_mutex_unlock(&mupdf_document->mutex);

  if (list != NULL) {
    girara_list_free(list);
  }

error_ret:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  return NULL;
}

/* text and character boxes of a page; the first pass only counts them, the second one fills them in */
typedef struct text_layout_s {
  char* text;                 /**< UTF-8 text, NULL while counting */
  zathura_rectangle_t* rects; /**< One box per character, NULL while counting */
  size_t length;              /**< Bytes of text */
  size_t n_rects;             /**< Number of characters */
} text_layout_t;

static void text_layout_append(text_layout_t* layout, int c, fz_rect rect) {
  if (layout->text != NULL) {
    layout->rects[layout->n_rects].x1 = rect.x0;
    layout->rects[layout->n_rects].y1 = rect.y0;
    layout->rects[layout->n_rects].x2 = rect.x1;
    layout->rects[layout->n_rects].y2 = rect.y1;
  }
  layout->length += g_unichar_to_utf8(c, layout->text != NULL ? layout->text + layout->length : NULL);
  layout->n_rects++;
}

static void text_layout_blocks(fz_stext_block* block, text_layout_t* layout) {
  for (; block != NULL; block = block->next) {
    /* structure blocks contain the blocks of tagged PDFs */
    if (block->type == FZ_STEXT_BLOCK_STRUCT) {
      if (block->u.s.down != NULL) {
        text_layout_blocks(block->u.s.down->first_block, layout);
      }
      continue;
    }
    if (block->type != FZ_STEXT_BLOCK_TEXT) {
      continue;
    }

    for (fz_stext_line* line = block->u.t.first_line; line != NULL; line = line->next) {
      fz_rect last = line->bbox;
      for (fz_stext_char* ch = line->first_char; ch != NULL; ch = ch->next) {
        /* skip invalid code points so that the text stays valid UTF-8 */
        if (g_unichar_validate(ch->c) == FALSE || ch->c == '\n') {
          continue;
        }
        last = fz_rect_from_quad(ch->quad);
        text_layout_append(layout, ch->c, last);
      }

      /* lines end with a newline without width, like in poppler */
      const fz_rect newline = {last.x1, last.y1, last.x1, last.y1};
      text_layout_append(layout, '\n', newline);
    }
  }
}

zathura_error_t pdf_page_get_text_layout(zathura_page_t* page, void* data, zathura_text_layout_t* layout) {
  mupdf_page_t* mupdf_page = data;
  if (page == NULL || mupdf_page == NULL || mupdf_page->text == NULL || layout == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  g_mutex_lock(&mupdf_document->mutex);
  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document, mupdf_page);
  }

  text_layout_t collected = {NULL, NULL, 0, 0};
  text_layout_blocks(mupdf_page->text->first_block, &collected);

  const size_t length  = collected.length;
  const size_t n_rects = collected.n_rects;
  collected.text       = g_malloc(length + 1);
  collected.rects      = g_malloc0(MAX(n_rects, 1) * sizeof(zathura_rectangle_t));
  collected.length     = 0;
  collected.n_rects    = 0;
  text_layout_blocks(mupdf_page->text->first_block, &collected);
  g_mutex_unlock(&mupdf_document->mutex);

  collected.text[length] = '\0';
  layout->text           = collected.text;
  layout->rects          = collected.rects;
  layout->n_rects        = n_rects;
  return ZATHURA_ERROR_OK;
}
