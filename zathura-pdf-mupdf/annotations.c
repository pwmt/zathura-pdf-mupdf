/* See LICENSE file for license and copyright information */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "plugin.h"
#include "internal.h"
#include "macros.h"
#include "utils.h"

static zathura_error_t mupdf_annotation_to_zathura_annotation(zathura_page_t*
    page, mupdf_document_t* mupdf_document, pdf_annot*
    mupdf_annot, zathura_annotation_t** annotation);

zathura_error_t
pdf_page_get_annotations(zathura_page_t* page, zathura_list_t** annotations)
{
  if (page == NULL || annotations == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;
  *annotations = NULL;

  zathura_document_t* document;
  if (zathura_page_get_document(page, &document) != ZATHURA_ERROR_OK || document == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  mupdf_document_t* mupdf_document;
  if ((error = zathura_document_get_user_data(document, (void**) &mupdf_document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_page_t* mupdf_page;
  if ((error = zathura_page_get_user_data(page, (void**) &mupdf_page)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  pdf_annot* mupdf_annotation = pdf_first_annot(mupdf_document->ctx, (pdf_page*) mupdf_page->page);
  while (mupdf_annotation != NULL) {
    zathura_annotation_t* annotation;
    if (mupdf_annotation_to_zathura_annotation(page, mupdf_document, mupdf_annotation, &annotation) !=
        ZATHURA_ERROR_OK) {
      mupdf_annotation = pdf_next_annot(mupdf_document->ctx, mupdf_annotation);
      continue;
    }

    fz_rect bounding_box;
    pdf_bound_annot(mupdf_document->ctx, mupdf_annotation, &bounding_box);

    zathura_rectangle_t position = {
      {bounding_box.x0, bounding_box.y0},
      {bounding_box.x1, bounding_box.y1}
    };

    if ((error = zathura_annotation_set_position(annotation, position)) != ZATHURA_ERROR_OK) {
      break;
    }

    *annotations = zathura_list_append(*annotations, annotation);

    /* next annot */
    mupdf_annotation = pdf_next_annot(mupdf_document->ctx, mupdf_annotation);
  }

  return error;

error_out:

  return error;
}

static zathura_error_t
mupdf_annotation_to_zathura_annotation(zathura_page_t* page, mupdf_document_t*
    mupdf_document, pdf_annot* mupdf_annotation,
    zathura_annotation_t** annotation)
{
  fz_annot_type mupdf_type = pdf_annot_type(mupdf_document->ctx, mupdf_annotation);
  zathura_annotation_type_t zathura_type = ZATHURA_ANNOTATION_UNKNOWN;

  zathura_error_t error = ZATHURA_ERROR_OK;

  typedef struct annotation_type_mapping_s {
    fz_annot_type mupdf;
    zathura_annotation_type_t zathura;
  } annotation_type_mapping_t;

  annotation_type_mapping_t type_mapping[] = {
    { FZ_ANNOT_TEXT,           ZATHURA_ANNOTATION_TEXT },
    { FZ_ANNOT_FREETEXT,       ZATHURA_ANNOTATION_FREE_TEXT },
    { FZ_ANNOT_LINE,           ZATHURA_ANNOTATION_LINE },
    { FZ_ANNOT_SQUARE,         ZATHURA_ANNOTATION_SQUARE },
    { FZ_ANNOT_CIRCLE,         ZATHURA_ANNOTATION_CIRCLE },
    { FZ_ANNOT_POLYGON,        ZATHURA_ANNOTATION_POLYGON },
    { FZ_ANNOT_POLYLINE,       ZATHURA_ANNOTATION_POLY_LINE },
    { FZ_ANNOT_HIGHLIGHT,      ZATHURA_ANNOTATION_HIGHLIGHT },
    { FZ_ANNOT_UNDERLINE,      ZATHURA_ANNOTATION_UNDERLINE },
    { FZ_ANNOT_SQUIGGLY,       ZATHURA_ANNOTATION_SQUIGGLY },
    { FZ_ANNOT_STRIKEOUT,      ZATHURA_ANNOTATION_STRIKE_OUT },
    { FZ_ANNOT_STAMP,          ZATHURA_ANNOTATION_STAMP },
    { FZ_ANNOT_CARET,          ZATHURA_ANNOTATION_CARET },
    { FZ_ANNOT_INK,            ZATHURA_ANNOTATION_INK },
    { FZ_ANNOT_POPUP,          ZATHURA_ANNOTATION_POPUP },
    { FZ_ANNOT_FILEATTACHMENT, ZATHURA_ANNOTATION_FILE_ATTACHMENT },
    { FZ_ANNOT_SOUND,          ZATHURA_ANNOTATION_SOUND },
    { FZ_ANNOT_MOVIE,          ZATHURA_ANNOTATION_MOVIE },
    { FZ_ANNOT_WIDGET,         ZATHURA_ANNOTATION_WIDGET },
    { FZ_ANNOT_SCREEN,         ZATHURA_ANNOTATION_SCREEN },
    { FZ_ANNOT_PRINTERMARK,    ZATHURA_ANNOTATION_PRINTER_MARK },
    { FZ_ANNOT_TRAPNET,        ZATHURA_ANNOTATION_TRAP_NET },
    { FZ_ANNOT_WATERMARK,      ZATHURA_ANNOTATION_WATERMARK },
    { FZ_ANNOT_3D,             ZATHURA_ANNOTATION_3D }
  };

  for (unsigned int i = 0; i < LENGTH(type_mapping); i++) {
    if (type_mapping[i].mupdf == mupdf_type) {
      zathura_type = type_mapping[i].zathura;
      break;
    }
  }

  /* create new annotation */
  if ((error = zathura_annotation_new(page, annotation, zathura_type)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  if ((error = zathura_annotation_set_user_data(*annotation, mupdf_annotation, NULL)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  /* set general properties */
  char* content = pdf_annot_contents(mupdf_document->ctx, (pdf_document*) mupdf_document->document, mupdf_annotation);
  if (content != NULL && (error = zathura_annotation_set_content(*annotation, content) != ZATHURA_ERROR_OK)) {
    goto error_free;
  }

  /* Check if annotation has an appearance stream */
  bool has_appearance_stream = false;
  if (mupdf_annotation->ap != NULL) {
    has_appearance_stream = true;
  }

  if ((error = zathura_annotation_set_appearance_stream(*annotation, has_appearance_stream)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  /* Check opacity */
  float opacity = 1.0;
  pdf_obj* obj = pdf_dict_get(mupdf_document->ctx, mupdf_annotation->obj, PDF_NAME_CA);
  if (pdf_is_number(mupdf_document->ctx, obj)) {
    opacity = pdf_to_real(mupdf_document->ctx, obj);
  }

  if ((error = zathura_annotation_set_opacity(*annotation, opacity)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  /* Check blend mode */
  obj = pdf_dict_get(mupdf_document->ctx, mupdf_annotation->obj, PDF_NAME_BM);
  if (pdf_is_array(mupdf_document->ctx, obj)) {
    obj = pdf_array_get(mupdf_document->ctx, obj, 0);
  }

  if (pdf_is_name(mupdf_document->ctx, obj)) {
    char* blend_mode_str = pdf_to_name(mupdf_document->ctx, obj);

    zathura_blend_mode_t blend_mode = mupdf_blend_mode_to_zathura_blend_mode(blend_mode_str);
    if ((error = zathura_annotation_set_blend_mode(*annotation, blend_mode)) != ZATHURA_ERROR_OK) {
      goto error_out;
    }
  }

  return error;

error_free:

    zathura_annotation_free(*annotation);

error_out:

    return error;
}

static void argb_to_rgb(fz_context *ctx, fz_colorspace *colorspace, const float *argb, float *rgb)
{
  (void) ctx;
  (void) colorspace;

  rgb[0] = argb[0];
  rgb[1] = argb[1];
  rgb[2] = argb[2];
}

static void rgb_to_argb(fz_context *ctx, fz_colorspace *colorspace, const float *rgb, float *argb)
{
  (void) ctx;
  (void) colorspace;

  argb[0] = rgb[2];
  argb[1] = rgb[1];
  argb[2] = rgb[0];
  argb[3] = 255;
}

static zathura_error_t
pdf_annotation_render_to_buffer(pdf_annot* mupdf_annotation, mupdf_document_t* mupdf_document, mupdf_page_t* mupdf_page,
			  unsigned char* image,
			  unsigned int annotation_width, unsigned int annotation_height,
        zathura_rectangle_t position,
			  double scalex, double scaley, cairo_format_t cairo_format)
{
  if (mupdf_annotation == NULL ||
      mupdf_document == NULL ||
      mupdf_document->ctx == NULL ||
      mupdf_page == NULL ||
      mupdf_page->page == NULL ||
      image == NULL) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  fz_display_list* display_list = fz_new_display_list(mupdf_page->ctx);
  fz_device* device             = fz_new_list_device(mupdf_page->ctx, display_list);

  fz_try (mupdf_document->ctx) {
    fz_matrix m;
    fz_scale(&m, scalex, scaley);
    pdf_run_annot(mupdf_document->ctx, mupdf_annotation, device, &m, NULL);
  } fz_catch (mupdf_document->ctx) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  fz_drop_device(mupdf_page->ctx, device);

  /* Prepare rendering */
  fz_irect irect = { .x1 = annotation_width, .y1 = annotation_height};

  fz_rect rect;
  rect.x0 = position.p1.x * scalex;
  rect.y0 = position.p1.y * scaley;
  rect.x1 = position.p2.x * scalex;
  rect.y1 = position.p2.y * scaley;

  fz_rect_from_irect(&rect, fz_round_rect(&irect, &rect));

  /* Create correct pixmap */
  fz_pixmap* pixmap = NULL;

  if (cairo_format == CAIRO_FORMAT_RGB24) {
    fz_colorspace* colorspace = fz_device_bgr(mupdf_document->ctx);
    pixmap = fz_new_pixmap_with_bbox_and_data(mupdf_page->ctx, colorspace, &irect, image);
  } else if (cairo_format == CAIRO_FORMAT_ARGB32) {
    /* Define new color space */
    fz_colorspace* colorspace = fz_new_colorspace(mupdf_document->ctx, "argb", 3);
    colorspace->to_rgb = argb_to_rgb;
    colorspace->from_rgb = rgb_to_argb;

    /* Create pixmap */
    pixmap = fz_new_pixmap_with_bbox_and_data(mupdf_page->ctx, colorspace, &irect, image);
  }

  device = fz_new_draw_device(mupdf_page->ctx, pixmap);
  fz_run_display_list(mupdf_page->ctx, display_list, device, &fz_identity, &rect, NULL);
  fz_drop_device(mupdf_page->ctx, device);

  fz_drop_pixmap(mupdf_page->ctx, pixmap);
  fz_drop_display_list(mupdf_page->ctx, display_list);

  return ZATHURA_ERROR_OK;
}

#ifdef HAVE_CAIRO
zathura_error_t pdf_annotation_render_cairo(zathura_annotation_t* annotation, cairo_t* cairo,
    double scale)
{
  if (annotation == NULL || cairo == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;

  cairo_surface_t* surface = cairo_get_target(cairo);
  if (surface == NULL ||
      cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS ||
      cairo_surface_get_type(surface) != CAIRO_SURFACE_TYPE_IMAGE) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  cairo_format_t cairo_format = cairo_image_surface_get_format(surface);
  if (cairo_format != CAIRO_FORMAT_ARGB32 && cairo_format != CAIRO_FORMAT_RGB24) {
    error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    goto error_out;
  }

  pdf_annot* mupdf_annotation;
  if ((error = zathura_annotation_get_user_data(annotation, (void**) &mupdf_annotation)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  zathura_page_t* page;
  if (zathura_annotation_get_page(annotation, &page) != ZATHURA_ERROR_OK || page == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  zathura_document_t* document;
  if (zathura_page_get_document(page, &document) != ZATHURA_ERROR_OK || document == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  mupdf_document_t* mupdf_document;
  if (zathura_document_get_user_data(document, (void**) &mupdf_document) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_page_t* mupdf_page;
  if ((error = zathura_page_get_user_data(page, (void**) &mupdf_page)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  zathura_rectangle_t position;
  if (zathura_annotation_get_position(annotation, &position) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  cairo_surface_flush(surface);

  unsigned int annotation_width  = cairo_image_surface_get_width(surface);
  unsigned int annotation_height = cairo_image_surface_get_height(surface);

  unsigned char* image = cairo_image_surface_get_data(surface);

  error = pdf_annotation_render_to_buffer(mupdf_annotation, mupdf_document,
      mupdf_page, image, annotation_width, annotation_height,
      position, scale, scale, cairo_format);

  cairo_surface_mark_dirty(surface);

  return error;

error_out:

  return error;
}
#endif
