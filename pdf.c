/* See LICENSE file for license and copyright information */

#include <stdlib.h>
#include <ctype.h>
#include <girara/datastructures.h>

#include "pdf.h"

/* forward declarations */
static inline int text_span_char_at(fz_text_span *span, int index);
static unsigned int text_span_length(fz_text_span *span);
static int text_span_match_string_n(fz_text_span* span, const char* string,
    int n, zathura_rectangle_t* rectangle);
static void pdf_zathura_image_free(zathura_image_t* image);
static void get_images(fz_obj* dict, girara_list_t* list);
static void get_resources(fz_obj* resource, girara_list_t* list);
static void search_result_add_char(zathura_rectangle_t* rectangle,
    fz_text_span* span, int n);
static void mupdf_page_extract_text(pdf_xref* document,
    mupdf_page_t* mupdf_page);

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
  document->functions.page_images_get           = pdf_page_images_get;
  document->functions.page_get_text             = pdf_page_get_text;
  document->functions.page_render               = pdf_page_render;
#if HAVE_CAIRO
  document->functions.page_render_cairo         = pdf_page_render_cairo;
#endif
  document->functions.page_free                 = pdf_page_free;

  document->data = malloc(sizeof(pdf_document_t));
  if (document->data == NULL) {
    error = ZATHURA_PLUGIN_ERROR_OUT_OF_MEMORY;
    goto error_ret;
  }

  pdf_document_t* pdf_document = (pdf_document_t*) document->data;

  fz_accelerate();
  pdf_document->glyph_cache = fz_new_glyph_cache();

  if (pdf_open_xref(&(pdf_document->document), document->file_path, NULL) != fz_okay) {
    error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
    goto error_free;
  }

  if (pdf_needs_password(pdf_document->document) != 0) {
    if (document->password == NULL || pdf_authenticate_password(pdf_document->document, (char*) document->password) != 0) {
      error = ZATHURA_PLUGIN_ERROR_INVALID_PASSWORD;
      goto error_free;
    }
  }

  if (pdf_load_page_tree(pdf_document->document) != fz_okay) {
    error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
    goto error_free;
  }

  document->number_of_pages = pdf_count_pages(pdf_document->document);

  return error;

error_free:

  if (pdf_document->document != NULL) {
    pdf_free_xref(pdf_document->document);
  }

  if (pdf_document->glyph_cache != NULL) {
    fz_free_glyph_cache(pdf_document->glyph_cache);
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

  pdf_document_t* pdf_document = (pdf_document_t*) document->data;
  pdf_free_xref(pdf_document->document);
  fz_free_glyph_cache(pdf_document->glyph_cache);
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

  pdf_document_t* pdf_document  = (pdf_document_t*) document->data;
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

  /* load page */
  if (pdf_load_page(&(mupdf_page->page), pdf_document->document, page) != fz_okay) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
    }
    goto error_free;
  }

  /* get page dimensions */
  document_page->width  = mupdf_page->page->mediabox.x1 - mupdf_page->page->mediabox.x0;
  document_page->height = mupdf_page->page->mediabox.y1 - mupdf_page->page->mediabox.y0;

  /* setup text */
  mupdf_page->extracted_text = false;
  mupdf_page->text = fz_new_text_span();
  if (mupdf_page->text == NULL) {
    goto error_free;
  }

  return document_page;

error_free:

  if (mupdf_page != NULL) {
    if (mupdf_page->page != NULL) {
      pdf_free_page(mupdf_page->page);
    }

    if (mupdf_page->text != NULL) {
      fz_free_text_span(mupdf_page->text);
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

  mupdf_page_t* mupdf_page = (mupdf_page_t*) page->data;
  pdf_free_page(mupdf_page->page);
  free(mupdf_page);
  free(page);

  return ZATHURA_PLUGIN_ERROR_OK;
}

girara_list_t*
pdf_page_search_text(zathura_page_t* page, const char* text, zathura_plugin_error_t* error)
{
  if (page == NULL || page->data == NULL || page->document == NULL ||
      page->document->data == NULL || text == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  mupdf_page_t* mupdf_page     = (mupdf_page_t*) page->data;
  pdf_document_t* pdf_document = (pdf_document_t*) page->document->data;

  if (mupdf_page->text == NULL) {
    goto error_ret;
  }

  /* extract text (only once) */
  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(pdf_document->document, mupdf_page);
  }


  girara_list_t* list = girara_list_new2((girara_free_function_t) zathura_link_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  unsigned int length = text_span_length(mupdf_page->text);
  for (int i = 0; i < length; i++) {
    zathura_rectangle_t* rectangle = g_malloc0(sizeof(zathura_rectangle_t));

    /* search for string */
    int match = text_span_match_string_n(mupdf_page->text, text, i, rectangle);
    if (match == 0) {
      g_free(rectangle);
      continue;
    }

    rectangle->y1 = page->height - rectangle->y1;
    rectangle->y2 = page->height - rectangle->y2;

    girara_list_append(list, rectangle);
  }

  return list;

error_free:

  if (list != NULL ) {
    girara_list_free(list);
  }

error_ret:

  if (error != NULL && *error == ZATHURA_PLUGIN_ERROR_OK) {
    *error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
  }

  return NULL;
}

girara_list_t*
pdf_page_links_get(zathura_page_t* page, zathura_plugin_error_t* error)
{
  if (page == NULL || page->data == NULL || page->document->data == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  pdf_document_t* pdf_document = (pdf_document_t*) page->document->data;
  mupdf_page_t* mupdf_page     = (mupdf_page_t*) page->data;

  if (mupdf_page->page == NULL) {
    goto error_ret;
  }

  girara_list_t* list = girara_list_new2((girara_free_function_t) zathura_link_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  pdf_link* link = mupdf_page->page->links;

  for (; link != NULL; link = link->next) {
    zathura_link_t* zathura_link = g_malloc0(sizeof(zathura_link_t));
    if (zathura_link == NULL) {
      if (error != NULL) {
        *error = ZATHURA_PLUGIN_ERROR_OUT_OF_MEMORY;
      }
      goto error_free;
    }

    /* extract position */
    zathura_link->position.x1 = link->rect.x0;
    zathura_link->position.x2 = link->rect.x1;
    zathura_link->position.y1 = page->height - link->rect.y1;
    zathura_link->position.y2 = page->height - link->rect.y0;

    if (link->kind == PDF_LINK_URI) {
      char* buffer = g_malloc0(sizeof(char) * (fz_to_str_len(link->dest) + 1));
      memcpy(buffer, fz_to_str_buf(link->dest), fz_to_str_len(link->dest));
      buffer[fz_to_str_len(link->dest)] = '\0';

      zathura_link->type         = ZATHURA_LINK_EXTERNAL;
      zathura_link->target.value = buffer;
    } else if (link->kind == PDF_LINK_GOTO) {
      int page_number = pdf_find_page_number(pdf_document->document, fz_array_get(link->dest, 0));

      zathura_link->type               = ZATHURA_LINK_TO_PAGE;
      zathura_link->target.page_number = page_number;
    } else {
      g_free(zathura_link);
      continue;
    }

    girara_list_append(list, zathura_link);
  }

  return list;

error_free:

  if (list != NULL) {
    girara_list_free(list);
  }

error_ret:

  return NULL;
}

char*
pdf_page_get_text(zathura_page_t* page, zathura_rectangle_t rectangle, zathura_plugin_error_t* error)
{
  if (page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  mupdf_page_t* mupdf_page = (mupdf_page_t*) page->data;

  if (mupdf_page->text == NULL) {
    goto error_ret;
  }

  GString* text = g_string_new(NULL);

  for (fz_text_span* span = mupdf_page->text; span != NULL; span = span->next) {
    bool seen = false;

    for (int i = 0; i < span->len; i++) {
      fz_bbox hitbox = fz_transform_bbox(fz_identity, span->text[i].bbox);
      int c = span->text[i].c;

      if (c < 32) {
        c = '?';
      }

      if (hitbox.x1 >= rectangle.x1
          && hitbox.x0 <= rectangle.x2
          && (page->height - hitbox.y1) >= rectangle.y1
          && (page->height - hitbox.y0) <= rectangle.y2) {
        g_string_append_c(text, c);
        seen = true;
      }
    }

    if (seen == true && span->eol != 0) {
      g_string_append_c(text, '\n');
    }
  }

  if (text->len == 0) {
    g_string_free(text, TRUE);
    return NULL;
  } else {
    char* t = text->str;
    g_string_free(text, FALSE);
    return t;
  }

error_ret:

  if (error != NULL && *error == ZATHURA_PLUGIN_ERROR_OK) {
    *error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
  }

  return NULL;
}

girara_list_t*
pdf_page_images_get(zathura_page_t* page, zathura_plugin_error_t* error)
{
  if (page == NULL || page->data == NULL || page->document == NULL ||
      page->document->data == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  pdf_document_t* pdf_document = (pdf_document_t*) page->document->data;

  fz_obj* page_object = pdf_document->document->page_objs[page->number];
  if (page_object == NULL) {
    goto error_free;
  }

  fz_obj* resource = fz_dict_gets(page_object, "Resources");
  if (resource == NULL) {
    goto error_free;
  }

  girara_list_t* list = girara_list_new();
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  girara_list_set_free_function(list, (girara_free_function_t) pdf_zathura_image_free);

  get_resources(resource, list);

  return list;

error_free:

  if (error != NULL && *error == ZATHURA_PLUGIN_ERROR_OK) {
    *error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
  }

  if (list != NULL) {
    girara_list_free(list);
  }

error_ret:

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

  pdf_document_t* pdf_document = (pdf_document_t*) page->document->data;
  mupdf_page_t* mupdf_page     = (mupdf_page_t*) page->data;

  /* render */
  fz_display_list* display_list = fz_new_display_list();
  fz_device* device             = fz_new_list_device(display_list);

  if (pdf_run_page(pdf_document->document, mupdf_page->page, device, fz_scale(page->document->scale, page->document->scale)) != fz_okay) {
    if (error != NULL) {
      *error = ZATHURA_PLUGIN_ERROR_UNKNOWN;
    }
    return NULL;
  }

  fz_free_device(device);

  fz_bbox bbox = { .x1 = page_width, .y1 = page_height };

  fz_pixmap* pixmap = fz_new_pixmap_with_rect(fz_device_rgb, bbox);
  fz_clear_pixmap_with_color(pixmap, 0xFF);

  device = fz_new_draw_device(pdf_document->glyph_cache, pixmap);
  fz_execute_display_list(display_list, device, fz_identity, bbox);
  fz_free_device(device);

  unsigned char* s = pixmap->samples;
  for (unsigned int y = 0; y < pixmap->h; y++) {
    for (unsigned int x = 0; x < pixmap->w; x++) {
      guchar* p = image_buffer->data + (pixmap->h - y - 1) *
        image_buffer->rowstride + x * 3;
      p[0] = s[0];
      p[1] = s[1];
      p[2] = s[2];
      s += pixmap->n;
    }
  }

  fz_drop_pixmap(pixmap);
  fz_free_display_list(display_list);

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
  double scalex = ((double)page_width) / page->width,
         scaley = ((double)page_height) / page->height;

  pdf_document_t* pdf_document = (pdf_document_t*) page->document->data;
  mupdf_page_t* mupdf_page     = (mupdf_page_t*) page->data;

  /* render */
  fz_display_list* display_list = fz_new_display_list();
  fz_device* device             = fz_new_list_device(display_list);

  if (pdf_run_page(pdf_document->document, mupdf_page->page, device, fz_scale(scalex, scaley)) != fz_okay) {
    return ZATHURA_PLUGIN_ERROR_UNKNOWN;
  }

  fz_free_device(device);


  fz_bbox bbox = { .x1 = page_width, .y1 = page_height };

  fz_pixmap* pixmap = fz_new_pixmap_with_rect(fz_device_rgb, bbox);
  fz_clear_pixmap_with_color(pixmap, 0xFF);

  device = fz_new_draw_device(pdf_document->glyph_cache, pixmap);
  fz_execute_display_list(display_list, device, fz_identity, bbox);
  fz_free_device(device);

  int rowstride        = cairo_image_surface_get_stride(surface);
  unsigned char* image = cairo_image_surface_get_data(surface);

  unsigned char *s = pixmap->samples;
  for (unsigned int y = 0; y < pixmap->h; y++) {
    for (unsigned int x = 0; x < pixmap->w; x++) {
      guchar* p = image + (pixmap->h - y - 1) * rowstride + x * 4;
      p[0] = s[0];
      p[1] = s[1];
      p[2] = s[2];
      s += pixmap->n;
    }
  }

  fz_drop_pixmap(pixmap);
  fz_free_display_list(display_list);

  return ZATHURA_PLUGIN_ERROR_OK;;
}
#endif

static inline int
text_span_char_at(fz_text_span *span, int index)
{
  int offset = 0;
  while (span != NULL) {
    if (index < offset + span->len) {
      return span->text[index - offset].c;
    }

    if (span->eol != 0) {
      if (index == offset + span->len) {
        return ' ';
      }
      offset++;
    }

    offset += span->len;
    span = span->next;
  }

  return 0;
}

static unsigned int
text_span_length(fz_text_span *span)
{
  unsigned int length = 0;

  while (span != NULL) {
    length += span->len;

    if (span->eol != 0) {
      length++;
    }

    span = span->next;
  }

  return length;
}

static int
text_span_match_string_n(fz_text_span* span, const char* string, int n,
    zathura_rectangle_t* rectangle)
{
  if (span == NULL || string == NULL || rectangle == NULL) {
    return 0;
  }

  int o = n;
  int c;

  while ((c = *string++)) {
    if (c == ' ' && text_span_char_at(span, n) == ' ') {
      while (text_span_char_at(span, n) == ' ') {
        search_result_add_char(rectangle, span, n);
        n++;
      }
    } else {
      if (tolower(c) != tolower(text_span_char_at(span, n))) {
        return 0;
      }
      search_result_add_char(rectangle, span, n);
      n++;
    }
  }

  return n - o;
}

static void
pdf_zathura_image_free(zathura_image_t* image)
{
  if (image == NULL) {
    return;
  }

  g_free(image);
}

static void
get_images(fz_obj* dict, girara_list_t* list)
{
  if (dict == NULL || list == NULL) {
    return;
  }

  for (int i = 0; i < fz_dict_len(dict); i++) {
    fz_obj* image_dict = fz_dict_get_val(dict, i);
    if (fz_is_dict(image_dict) == 0) {
      continue;
    }

    fz_obj* type = fz_dict_gets(image_dict, "Subtype");
    if (strcmp(fz_to_name(type), "Image") != 0) {
      continue;
    }

    bool duplicate = false;
    GIRARA_LIST_FOREACH(list, zathura_image_t*, iter, image)
      if (image->data == image_dict) {
        duplicate = true;
        break;
      }
    GIRARA_LIST_FOREACH_END(list, zathura_image_t*, iter, image);

    if (duplicate == true) {
      continue;
    }

    fz_obj* width  = fz_dict_gets(image_dict, "Width");
    fz_obj* height = fz_dict_gets(image_dict, "Height");

    zathura_image_t* zathura_image = g_malloc(sizeof(zathura_image_t));

    // FIXME: Get correct image coordinates
    zathura_image->data        = image_dict;
    zathura_image->position.x1 = 0;
    zathura_image->position.x2 = fz_to_int(width);
    zathura_image->position.y1 = 0;
    zathura_image->position.y2 = fz_to_int(height);

    girara_list_append(list, zathura_image);
  }
}

static void
get_resources(fz_obj* resource, girara_list_t* list)
{
  if (resource == NULL || list == NULL) {
    return;
  }

  fz_obj* x_object = fz_dict_gets(resource, "XObject");
  if (x_object == NULL) {
    return;
  }

  get_images(x_object, list);

  for (int i = 0; i < fz_dict_len(x_object); i++) {
    fz_obj* obj = fz_dict_get_val(x_object, i);
    fz_obj* subsrc = fz_dict_gets(obj, "Resources");
    if (subsrc != NULL && fz_objcmp(resource, subsrc)) {
      get_resources(subsrc, list);
    }
  }
}

static void
search_result_add_char(zathura_rectangle_t* rectangle, fz_text_span* span,
    int index)
{
  if (rectangle == NULL || span == NULL) {
    return;
  }

  int offset = 0;
  for (; span != NULL; span = span->next) {
    if (index < offset + span->len) {
      fz_bbox coordinates = span->text[index - offset].bbox;

      if (rectangle->x1 == 0) {
        rectangle->x1 = coordinates.x0;
      }

      if (coordinates.x1 > rectangle->x2) {
        rectangle->x2 = coordinates.x1;
      }

      if (coordinates.y1 > rectangle->y1) {
        rectangle->y1 = coordinates.y1;
      }

      if (rectangle->y2 == 0) {
        rectangle->y2 = coordinates.y0;
      }

      return;
    }

    if (span->eol != 0) {
      offset++;
    }

    offset += span->len;
  }
}

static void
mupdf_page_extract_text(pdf_xref* document, mupdf_page_t* mupdf_page)
{
  if (document == NULL || mupdf_page == NULL || mupdf_page->extracted_text == true) {
    return;
  }

  fz_display_list* display_list = fz_new_display_list();
  fz_device* device             = fz_new_list_device(display_list);
  fz_device* text_device        = fz_new_text_device(mupdf_page->text);

  if (pdf_run_page(document, mupdf_page->page, device, fz_identity) != fz_okay) {
    return;
  }

  fz_execute_display_list(display_list, text_device, fz_identity, fz_infinite_bbox);
  mupdf_page->extracted_text = true;

  fz_free_device(text_device);
  fz_free_device(device);
  fz_free_display_list(display_list);
}
