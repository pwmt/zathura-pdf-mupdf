/* SPDX-License-Identifier: Zlib */

#include <mupdf/pdf.h>

#include "plugin.h"

static bool annotation_type_to_mupdf(zathura_annotation_type_t type, enum pdf_annot_type* result) {
  switch (type) {
  case ZATHURA_ANNOTATION_TEXT:
    *result = PDF_ANNOT_TEXT;
    return true;
  case ZATHURA_ANNOTATION_HIGHLIGHT:
    *result = PDF_ANNOT_HIGHLIGHT;
    return true;
  case ZATHURA_ANNOTATION_UNDERLINE:
    *result = PDF_ANNOT_UNDERLINE;
    return true;
  case ZATHURA_ANNOTATION_SQUIGGLY:
    *result = PDF_ANNOT_SQUIGGLY;
    return true;
  case ZATHURA_ANNOTATION_STRIKEOUT:
    *result = PDF_ANNOT_STRIKE_OUT;
    return true;
  default:
    return false;
  }
}

static zathura_annotation_type_t annotation_type_from_mupdf(enum pdf_annot_type type) {
  switch (type) {
  case PDF_ANNOT_TEXT:
    return ZATHURA_ANNOTATION_TEXT;
  case PDF_ANNOT_HIGHLIGHT:
    return ZATHURA_ANNOTATION_HIGHLIGHT;
  case PDF_ANNOT_UNDERLINE:
    return ZATHURA_ANNOTATION_UNDERLINE;
  case PDF_ANNOT_SQUIGGLY:
    return ZATHURA_ANNOTATION_SQUIGGLY;
  case PDF_ANNOT_STRIKE_OUT:
    return ZATHURA_ANNOTATION_STRIKEOUT;
  default:
    return ZATHURA_ANNOTATION_OTHER;
  }
}

/* annotations that are only parts of other annotations or are handled elsewhere */
static bool annotation_is_listed(enum pdf_annot_type type) {
  return type != PDF_ANNOT_POPUP && type != PDF_ANNOT_LINK && type != PDF_ANNOT_WIDGET;
}

static zathura_rectangle_t rect_from_fz(fz_rect rect) {
  return (zathura_rectangle_t){.x1 = rect.x0, .y1 = rect.y0, .x2 = rect.x1, .y2 = rect.y1};
}

static fz_rect rect_to_fz(zathura_rectangle_t rect) {
  return (fz_rect){.x0 = rect.x1, .y0 = rect.y1, .x1 = rect.x2, .y1 = rect.y2};
}

girara_list_t* pdf_page_annotations_get(zathura_page_t* page, void* data, zathura_error_t* error) {
  mupdf_page_t* mupdf_page = data;
  if (page == NULL || mupdf_page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    return NULL;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  girara_list_t* list = girara_list_new_with_free((girara_free_function_t)zathura_annotation_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    return NULL;
  }

  g_mutex_lock(&mupdf_document->mutex);
  fz_try(mupdf_page->ctx) {
    pdf_page* ppage = pdf_page_from_fz_page(mupdf_page->ctx, mupdf_page->page);
    unsigned int index = 0;
    for (pdf_annot* annot = ppage != NULL ? pdf_first_annot(mupdf_page->ctx, ppage) : NULL; annot != NULL;
         annot            = pdf_next_annot(mupdf_page->ctx, annot), ++index) {
      const enum pdf_annot_type type = pdf_annot_type(mupdf_page->ctx, annot);
      if (annotation_is_listed(type) == false) {
        continue;
      }

      zathura_annotation_t* result = g_try_malloc0(sizeof(zathura_annotation_t));
      if (result == NULL) {
        continue;
      }
      /* index counts all annotations of the page, including unlisted ones, so that it matches pdf_first_annot */
      result->index = index;
      result->type  = annotation_type_from_mupdf(type);
      result->rect  = rect_from_fz(pdf_bound_annot(mupdf_page->ctx, annot));

      if (pdf_annot_has_quad_points(mupdf_page->ctx, annot) != 0) {
        const int n = pdf_annot_quad_point_count(mupdf_page->ctx, annot);
        if (n > 0) {
          result->quads = g_try_malloc0_n((size_t)n, sizeof(zathura_rectangle_t));
          if (result->quads != NULL) {
            result->n_quads = (size_t)n;
            for (int i = 0; i < n; ++i) {
              result->quads[i] = rect_from_fz(fz_rect_from_quad(pdf_annot_quad_point(mupdf_page->ctx, annot, i)));
            }
          }
        }
      }

      int n_color = 0;
      float color[4];
      pdf_annot_color(mupdf_page->ctx, annot, &n_color, color);
      if (n_color == 3) {
        result->has_color = true;
        for (int i = 0; i < 3; ++i) {
          result->color[i] = color[i];
        }
      } else if (n_color == 1) {
        result->has_color = true;
        result->color[0] = result->color[1] = result->color[2] = color[0];
      }

      const char* contents = pdf_annot_contents(mupdf_page->ctx, annot);
      if (contents != NULL && contents[0] != '\0') {
        result->contents = g_strdup(contents);
      }
      if (pdf_annot_has_author(mupdf_page->ctx, annot) != 0) {
        const char* author = pdf_annot_author(mupdf_page->ctx, annot);
        if (author != NULL && author[0] != '\0') {
          result->author = g_strdup(author);
        }
      }

      girara_list_append(list, result);
    }
  }
  fz_catch(mupdf_page->ctx) {
    g_mutex_unlock(&mupdf_document->mutex);
    girara_list_free(list);
    if (error != NULL) {
      *error = ZATHURA_ERROR_UNKNOWN;
    }
    return NULL;
  }
  g_mutex_unlock(&mupdf_document->mutex);

  return list;
}

zathura_error_t pdf_page_annotation_add(zathura_page_t* page, void* data, const zathura_annotation_t* annotation) {
  mupdf_page_t* mupdf_page = data;
  if (page == NULL || mupdf_page == NULL || annotation == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  enum pdf_annot_type type;
  if (annotation_type_to_mupdf(annotation->type, &type) == false) {
    return ZATHURA_ERROR_NOT_IMPLEMENTED;
  }
  const bool markup = type != PDF_ANNOT_TEXT;
  if (markup == true && (annotation->quads == NULL || annotation->n_quads == 0)) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);
  zathura_error_t result           = ZATHURA_ERROR_OK;

  g_mutex_lock(&mupdf_document->mutex);
  fz_context* ctx = mupdf_page->ctx;
  pdf_page* ppage = NULL;
  fz_var(ppage);
  fz_var(result);
  fz_try(ctx) {
    ppage = pdf_page_from_fz_page(ctx, mupdf_page->page);
    if (ppage == NULL) {
      result = ZATHURA_ERROR_NOT_IMPLEMENTED;
    } else {
      pdf_begin_operation(ctx, ppage->doc, "Add annotation");
      fz_try(ctx) {
        /* owned by the page */
        pdf_annot* annot = pdf_create_annot(ctx, ppage, type);

        if (markup == true) {
          fz_quad* quads = fz_malloc_array(ctx, annotation->n_quads, fz_quad);
          fz_try(ctx) {
            for (size_t i = 0; i < annotation->n_quads; ++i) {
              quads[i] = fz_quad_from_rect(rect_to_fz(annotation->quads[i]));
            }
            pdf_set_annot_quad_points(ctx, annot, (int)annotation->n_quads, quads);
          }
          fz_always(ctx) {
            fz_free(ctx, quads);
          }
          fz_catch(ctx) {
            fz_rethrow(ctx);
          }
        } else {
          pdf_set_annot_rect(ctx, annot, rect_to_fz(annotation->rect));
        }

        if (annotation->has_color == true) {
          const float color[3] = {annotation->color[0], annotation->color[1], annotation->color[2]};
          pdf_set_annot_color(ctx, annot, 3, color);
        }
        if (annotation->contents != NULL) {
          pdf_set_annot_contents(ctx, annot, annotation->contents);
        }
        const char* user = g_get_real_name();
        if (user != NULL && user[0] != '\0' && g_strcmp0(user, "Unknown") != 0) {
          pdf_set_annot_author(ctx, annot, user);
        }
        pdf_set_annot_creation_date(ctx, annot, time(NULL));
        pdf_update_annot(ctx, annot);
      }
      fz_always(ctx) {
        pdf_end_operation(ctx, ppage->doc);
      }
      fz_catch(ctx) {
        fz_rethrow(ctx);
      }
    }
  }
  fz_catch(ctx) {
    result = ZATHURA_ERROR_UNKNOWN;
  }
  g_mutex_unlock(&mupdf_document->mutex);

  return result;
}

static pdf_annot* annotation_nth(fz_context* ctx, pdf_page* ppage, unsigned int index) {
  unsigned int i = 0;
  for (pdf_annot* annot = pdf_first_annot(ctx, ppage); annot != NULL; annot = pdf_next_annot(ctx, annot), ++i) {
    if (i == index) {
      return annot;
    }
  }
  return NULL;
}

zathura_error_t pdf_page_annotation_delete(zathura_page_t* page, void* data, unsigned int index) {
  mupdf_page_t* mupdf_page = data;
  if (page == NULL || mupdf_page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);
  zathura_error_t result           = ZATHURA_ERROR_OK;

  g_mutex_lock(&mupdf_document->mutex);
  fz_context* ctx = mupdf_page->ctx;
  fz_var(result);
  fz_try(ctx) {
    pdf_page* ppage = pdf_page_from_fz_page(ctx, mupdf_page->page);
    pdf_annot* annot   = ppage != NULL ? annotation_nth(ctx, ppage, index) : NULL;
    if (annot == NULL) {
      result = ZATHURA_ERROR_INVALID_ARGUMENTS;
    } else {
      pdf_begin_operation(ctx, ppage->doc, "Delete annotation");
      fz_try(ctx) {
        pdf_delete_annot(ctx, ppage, annot);
      }
      fz_always(ctx) {
        pdf_end_operation(ctx, ppage->doc);
      }
      fz_catch(ctx) {
        fz_rethrow(ctx);
      }
    }
  }
  fz_catch(ctx) {
    result = ZATHURA_ERROR_UNKNOWN;
  }
  g_mutex_unlock(&mupdf_document->mutex);

  return result;
}

zathura_error_t pdf_page_annotation_set_contents(zathura_page_t* page, void* data, unsigned int index,
                                                 const char* contents) {
  mupdf_page_t* mupdf_page = data;
  if (page == NULL || mupdf_page == NULL || contents == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);
  zathura_error_t result           = ZATHURA_ERROR_OK;

  g_mutex_lock(&mupdf_document->mutex);
  fz_context* ctx = mupdf_page->ctx;
  fz_var(result);
  fz_try(ctx) {
    pdf_page* ppage = pdf_page_from_fz_page(ctx, mupdf_page->page);
    pdf_annot* annot   = ppage != NULL ? annotation_nth(ctx, ppage, index) : NULL;
    if (annot == NULL) {
      result = ZATHURA_ERROR_INVALID_ARGUMENTS;
    } else {
      pdf_set_annot_contents(ctx, annot, contents);
      pdf_update_annot(ctx, annot);
    }
  }
  fz_catch(ctx) {
    result = ZATHURA_ERROR_UNKNOWN;
  }
  g_mutex_unlock(&mupdf_document->mutex);

  return result;
}
