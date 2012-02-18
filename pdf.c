/* See LICENSE file for license and copyright information */

#include <stdlib.h>
#include <girara/datastructures.h>

#include "pdf.h"

void
plugin_register(zathura_document_plugin_t* plugin)
{
  girara_list_append(plugin->content_types, g_content_type_from_mime_type("application/pdf"));
  plugin->open_function  = pdf_document_open;
}

zathura_plugin_error_t
pdf_document_open(zathura_document_t* document)
{
  zathura_plugin_error_t error = ZATHURA_PLUGIN_ERROR_OK;
  if (document == NULL) {
    error = ZATHURA_PLUGIN_ERROR_INVALID_ARGUMENTS;
    goto error_ret;
  }

  document->functions.document_free             = pdf_document_free;
  document->functions.page_get                  = pdf_page_get;
  document->functions.page_search_text          = pdf_page_search_text;
  document->functions.page_links_get            = pdf_page_links_get;
  document->functions.page_form_fields_get      = pdf_page_form_fields_get;
  document->functions.page_render               = pdf_page_render;
#if HAVE_CAIRO
  document->functions.page_render_cairo         = pdf_page_render_cairo;
#endif
  document->functions.page_free                 = pdf_page_free;

  document->data = malloc(sizeof(mupdf_document_t));
  if (document->data == NULL) {
    error = ZATHURA_PLUGIN_ERROR_OUT_OF_MEMORY;
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = (mupdf_document_t*) document->data;

  mupdf_document->ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
  if (mupdf_document->ctx == NULL) {
    error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
    goto error_free;
  }

  /* open document */
  if ((mupdf_document->document = (fz_document*) pdf_open_document(mupdf_document->ctx, document->file_path)) == NULL) {
    error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
    goto error_free;
  }

  /* authenticate if password is required and given */
  if (fz_needs_password(mupdf_document->document) != 0) {
    if (document->password == NULL || fz_authenticate_password(mupdf_document->document, (char*) document->password) != 0) {
      error = ZATHURA_PLUGIN_ERROR_INVALID_PASSWORD;
      goto error_free;
    }
  }

  document->number_of_pages = pdf_count_pages((pdf_document*) mupdf_document->document);

  return error;

error_free:

  if (mupdf_document->document != NULL) {
    fz_close_document(mupdf_document->document);
  }

  free(document->data);
  document->data = NULL;

error_ret:

  return error;
}

zathura_plugin_error_t
pdf_document_free(zathura_document_t* document)
{
  if (document == NULL || document->data == NULL) {
    return ZATHURA_PLUGIN_ERROR_INVALID_ARGUMENTS;
  }

  mupdf_document_t* mupdf_document = (mupdf_document_t*) document->data;
  fz_close_document(mupdf_document->document);
  free(document->data);
  document->data = NULL;

  return ZATHURA_PLUGIN_ERROR_OK;
}

zathura_page_t*
pdf_page_get(zathura_document_t* document, unsigned int page, zathura_plugin_error_t* error)
{
  if (document == NULL || document->data == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_INVALID_ARGUMENTS;
    }
    return NULL;
  }

  mupdf_document_t* mupdf_document  = (mupdf_document_t*) document->data;
  zathura_page_t* document_page = malloc(sizeof(zathura_page_t));

  if (document_page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_OUT_OF_MEMORY;
    }
    goto error_ret;
  }

  mupdf_page_t* mupdf_page = calloc(1, sizeof(mupdf_page_t));

  if (mupdf_page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  document_page->document = document;
  document_page->data     = mupdf_page;
  mupdf_page->ctx         = mupdf_document->ctx;

  /* load page */
  fz_try (mupdf_page->ctx) {
    mupdf_page->page = pdf_load_page((pdf_document*) mupdf_document->document, page);
  } fz_catch (mupdf_page->ctx) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
    }
    goto error_free;
  }

  /* get page dimensions */
  document_page->width  = mupdf_page->page->mediabox.x1 - mupdf_page->page->mediabox.x0;
  document_page->height = mupdf_page->page->mediabox.y1 - mupdf_page->page->mediabox.y0;

  return document_page;

error_free:

  if (mupdf_page != NULL) {
    if (mupdf_page->page != NULL) {
      pdf_free_page((pdf_document*) mupdf_document->document, mupdf_page->page);
    }

    free(mupdf_page);
  }

  if (document_page != NULL) {
    free(document_page);
  }

error_ret:

  return NULL;
}

zathura_plugin_error_t
pdf_page_free(zathura_page_t* page)
{
  if (page == NULL) {
    return ZATHURA_PLUGIN_ERROR_INVALID_ARGUMENTS;
  }

  if (page->data != NULL) {
    mupdf_page_t* mupdf_page = (mupdf_page_t*) page->data;
    if (mupdf_page->document != NULL) {
      pdf_free_page((pdf_document*) mupdf_page->document, mupdf_page->page);
    }
    free(mupdf_page);
  }

  free(page);

  return ZATHURA_PLUGIN_ERROR_OK;
}

#if 0
static int textlen(fz_text_span *span) {
  int text_length = 0;
  while (span != NULL) {
    text_length += span->len;
    if (span->eol) {
      text_length++;
    }
    span = span->next;
  }

  return text_length;
}
#endif

girara_list_t*
pdf_page_search_text(zathura_page_t* page, const char* text, zathura_plugin_error_t* error)
{
  if (page == NULL || page->data == NULL || text == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  mupdf_page_t* mupdf_page = (mupdf_page_t*) page->data;

  if (mupdf_page->ctx == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
    }
    goto error_ret;
  }

  fz_text_span* page_text    = fz_new_text_span(mupdf_page->ctx);
  fz_device* text_device     = fz_new_text_device(mupdf_page->ctx, page_text);
  fz_display_list* page_list = fz_new_display_list(mupdf_page->ctx);

  fz_run_display_list(page_list, text_device, fz_identity, fz_infinite_bbox, NULL);

  fz_free_display_list(mupdf_page->ctx, page_list);
  fz_free_device(text_device);
  fz_free_text_span(mupdf_page->ctx, page_text);

error_ret:

  return NULL;
}

girara_list_t*
pdf_page_links_get(zathura_page_t* page, zathura_plugin_error_t* error)
{
  if (error != NULL) {
    *error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
  }
  return NULL;
}

girara_list_t*
pdf_page_form_fields_get(zathura_page_t* page, zathura_plugin_error_t* error)
{
  if (error != NULL) {
    *error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
  }
  return NULL;
}

zathura_image_buffer_t*
pdf_page_render(zathura_page_t* page, zathura_plugin_error_t* error)
{
  if (page == NULL || page->data == NULL || page->document == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_INVALID_ARGUMENTS;
    }
    return NULL;
  }

  /* calculate sizes */
  unsigned int page_width  = page->document->scale * page->width;
  unsigned int page_height = page->document->scale * page->height;

  /* create image buffer */
  zathura_image_buffer_t* image_buffer = zathura_image_buffer_create(page_width, page_height);

  if (image_buffer == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_OUT_OF_MEMORY;
    }
    return NULL;
  }

  mupdf_document_t* mupdf_document = (mupdf_document_t*) page->document->data;
  mupdf_page_t* mupdf_page         = (mupdf_page_t*) page->data;

  /* render */
  fz_display_list* display_list = fz_new_display_list(mupdf_page->ctx);
  fz_device* device             = fz_new_list_device(mupdf_page->ctx, display_list);

  fz_try (mupdf_document->ctx) {
    pdf_run_page((pdf_document*) mupdf_document->document, mupdf_page->page, device, fz_scale(page->document->scale, page->document->scale), NULL);
  } fz_catch (mupdf_document->ctx) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_OUT_OF_MEMORY;
    }
    return NULL;
  }

  fz_free_device(device);

  fz_bbox bbox = { .x1 = page_width, .y1 = page_height };

  fz_pixmap* pixmap = fz_new_pixmap_with_rect(mupdf_page->ctx, fz_device_rgb, bbox);
  fz_clear_pixmap_with_value(mupdf_page->ctx, pixmap, 0xFF);

  device = fz_new_draw_device(mupdf_page->ctx, pixmap);
  fz_run_display_list(display_list, device, fz_identity, bbox, NULL);
  fz_free_device(device);

  unsigned char* s = pixmap->samples;
  for (unsigned int y = 0; y < pixmap->h; y++) {
    for (unsigned int x = 0; x < pixmap->w; x++) {
      guchar* p = image_buffer->data + y * image_buffer->rowstride + x * 3;
      p[0] = s[0];
      p[1] = s[1];
      p[2] = s[2];
      s += pixmap->n;
    }
  }

  fz_drop_pixmap(mupdf_page->ctx, pixmap);
  fz_free_display_list(mupdf_page->ctx, display_list);

  return image_buffer;
}

#if HAVE_CAIRO
zathura_plugin_error_t
pdf_page_render_cairo(zathura_page_t* page, cairo_t* cairo, bool GIRARA_UNUSED(printing))
{
  if (page == NULL || page->data == NULL || page->document == NULL) {
    return ZATHURA_PLUGIN_ERROR_INVALID_ARGUMENTS;
  }

  cairo_surface_t* surface = cairo_get_target(cairo);
  if (surface == NULL) {
    return ZATHURA_PLUGIN_ERROR_UNKNOWN;
  }

  unsigned int page_width  = cairo_image_surface_get_width(surface);
  unsigned int page_height = cairo_image_surface_get_height(surface);
  double scalex = ((double) page_width)  / page->width;
  double scaley = ((double) page_height) / page->height;

  mupdf_document_t* mupdf_document = (mupdf_document_t*) page->document->data;
  mupdf_page_t* mupdf_page         = (mupdf_page_t*) page->data;

  if (mupdf_document->ctx == NULL) {
    return ZATHURA_PLUGIN_ERROR_UNKNOWN;
  }

  /* render */
  fz_display_list* display_list = fz_new_display_list(mupdf_page->ctx);
  fz_device* device             = fz_new_list_device(mupdf_page->ctx, display_list);

  fz_try (mupdf_document->ctx) {
    pdf_run_page((pdf_document*) mupdf_document->document, mupdf_page->page, device, fz_scale(scalex, scaley), NULL);
  } fz_catch (mupdf_document->ctx) {
    return ZATHURA_PLUGIN_ERROR_UNKNOWN;
  }

  fz_free_device(device);

  fz_bbox bbox = { .x1 = page_width, .y1 = page_height };

  fz_pixmap* pixmap = fz_new_pixmap_with_rect(mupdf_page->ctx, fz_device_rgb, bbox);
  fz_clear_pixmap_with_value(mupdf_page->ctx, pixmap, 0xFF);

  device = fz_new_draw_device(mupdf_page->ctx, pixmap);
  fz_run_display_list(display_list, device, fz_identity, bbox, NULL);
  fz_free_device(device);

  int rowstride        = cairo_image_surface_get_stride(surface);
  unsigned char* image = cairo_image_surface_get_data(surface);

  unsigned char *s = pixmap->samples;
  for (unsigned int y = 0; y < pixmap->h; y++) {
    for (unsigned int x = 0; x < pixmap->w; x++) {
      guchar* p = image + y * rowstride + x * 4;
      p[0] = s[0];
      p[1] = s[1];
      p[2] = s[2];
      s += pixmap->n;
    }
  }

  fz_drop_pixmap(mupdf_page->ctx, pixmap);
  fz_free_display_list(mupdf_page->ctx, display_list);

  return ZATHURA_PLUGIN_ERROR_OK;
}
#endif
