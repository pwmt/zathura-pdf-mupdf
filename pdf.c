/* See LICENSE file for license and copyright information */

#include <stdlib.h>
#include <ctype.h>
#include <girara/datastructures.h>
#include <glib.h>

#include "pdf.h"

/* forward declarations */
static inline int text_page_char_at(fz_text_page* page, int index);
static unsigned int text_page_length(fz_text_page* page);
static int text_page_match_string_n(fz_text_page* page, const char* string,
    int n, zathura_rectangle_t* rectangle);
static void search_result_add_char(zathura_rectangle_t* rectangle, fz_text_page*
  page, int n);
static void mupdf_page_extract_text(fz_document* document,
    mupdf_page_t* mupdf_page);
static void build_index(fz_outline* outline,
    girara_tree_node_t* root);
#if 0
static void pdf_zathura_image_free(zathura_image_t* image);
static void get_images(pdf_obj* dict, girara_list_t* list);
static void get_resources(pdf_obj* resource, girara_list_t* list);
#endif

void
register_functions(zathura_plugin_functions_t* functions)
{
  functions->document_open            = pdf_document_open;
  functions->document_free            = pdf_document_free;
  functions->document_index_generate  = pdf_document_index_generate;
  functions->page_init                = pdf_page_init;
  functions->page_clear               = pdf_page_clear;
  functions->page_search_text         = pdf_page_search_text;
  functions->page_links_get           = pdf_page_links_get;
#if 0
  functions->page_images_get          = pdf_page_images_get;
  functions->document_get_information = pdf_document_get_information;
#endif
  functions->page_get_text            = pdf_page_get_text;
  functions->page_render              = pdf_page_render;
#if HAVE_CAIRO
  functions->page_render_cairo        = pdf_page_render_cairo;
#endif
}

ZATHURA_PLUGIN_REGISTER(
  "pdf-mupdf",
  0, 1, 0,
  register_functions,
  ZATHURA_PLUGIN_MIMETYPES({
    "application/pdf"
  })
)

zathura_error_t
pdf_document_open(zathura_document_t* document)
{
  zathura_error_t error = ZATHURA_ERROR_OK;
  if (document == NULL) {
    error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = calloc(1, sizeof(mupdf_document_t));
  if (mupdf_document == NULL) {
    error = ZATHURA_ERROR_OUT_OF_MEMORY;
    goto error_ret;
  }

  mupdf_document->ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
  if (mupdf_document->ctx == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  /* open document */
  const char* path     = zathura_document_get_path(document);
  const char* password = zathura_document_get_password(document);

  if ((mupdf_document->document = (fz_document*) pdf_open_document(mupdf_document->ctx, path)) == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  /* authenticate if password is required and given */
  if (fz_needs_password(mupdf_document->document) != 0) {
    if (password == NULL || fz_authenticate_password(mupdf_document->document, (char*) password) != 0) {
      error = ZATHURA_ERROR_INVALID_PASSWORD;
      goto error_free;
    }
  }

  zathura_document_set_number_of_pages(document, pdf_count_pages((pdf_document*) mupdf_document->document));
  zathura_document_set_data(document, mupdf_document);

  return error;

error_free:

  if (mupdf_document != NULL) {
    if (mupdf_document->document != NULL) {
      fz_close_document(mupdf_document->document);
    }

    free(mupdf_document);
  }

  zathura_document_set_data(document, NULL);

error_ret:

  return error;
}

zathura_error_t
pdf_document_free(zathura_document_t* document, mupdf_document_t* mupdf_document)
{
  if (document == NULL || mupdf_document == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  fz_close_document(mupdf_document->document);
  free(mupdf_document);
  zathura_document_set_data(document, NULL);

  return ZATHURA_ERROR_OK;
}

girara_tree_node_t*
pdf_document_index_generate(zathura_document_t* document, mupdf_document_t* mupdf_document, zathura_error_t* error)
{
  if (document == NULL || mupdf_document == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    return NULL;
  }

  /* get outline */
  fz_outline* outline = fz_load_outline(mupdf_document->document);
  if (outline == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_UNKNOWN;
    }
    return NULL;
  }

  /* generate index */
  girara_tree_node_t* root = girara_node_new(zathura_index_element_new("ROOT"));
  build_index(outline, root);

  /* free outline */
  fz_free_outline(mupdf_document->ctx, outline);

  return root;
}

zathura_error_t
pdf_page_init(zathura_page_t* page)
{
  if (page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);
  mupdf_page_t* mupdf_page         = calloc(1, sizeof(mupdf_page_t));
  unsigned int index               = zathura_page_get_index(page);

  if (mupdf_page == NULL) {
    return  ZATHURA_ERROR_OUT_OF_MEMORY;
  }

  zathura_page_set_data(page, mupdf_page);

  mupdf_page->ctx = mupdf_document->ctx;
  if (mupdf_page->ctx == NULL) {
    goto error_free;
  }

  /* load page */
  fz_try (mupdf_page->ctx) {
    mupdf_page->page = (pdf_page*) fz_load_page(mupdf_document->document, index);
  } fz_catch (mupdf_page->ctx) {
    goto error_free;
  }

  mupdf_page->bbox = fz_bound_page(mupdf_document->document, (fz_page*) mupdf_page->page);

  /* get page dimensions */
  zathura_page_set_width(page,  mupdf_page->bbox.x1 - mupdf_page->bbox.x0);
  zathura_page_set_height(page, mupdf_page->bbox.y1 - mupdf_page->bbox.y0);

  /* setup text */
  mupdf_page->extracted_text = false;
  mupdf_page->text           = fz_new_text_page(mupdf_page->ctx, mupdf_page->bbox);
  if (mupdf_page->text == NULL) {
    goto error_free;
  }

  mupdf_page->sheet = fz_new_text_sheet(mupdf_page->ctx);
  if (mupdf_page->sheet == NULL) {
    goto error_free;
  }

  return ZATHURA_ERROR_OK;

error_free:

  pdf_page_clear(page, mupdf_page);

  return ZATHURA_ERROR_UNKNOWN;
}

zathura_error_t
pdf_page_clear(zathura_page_t* page, mupdf_page_t* mupdf_page)
{
  if (page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_document_t* document     = zathura_page_get_document(page);
  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  if (mupdf_page != NULL) {
    if (mupdf_page->text != NULL) {
      fz_free_text_page(mupdf_page->ctx, mupdf_page->text);
    }

    if (mupdf_page->sheet != NULL) {
      fz_free_text_sheet(mupdf_page->ctx, mupdf_page->sheet);
    }

    if (mupdf_page->page != NULL) {
      pdf_free_page((pdf_document*) mupdf_document->document, mupdf_page->page);
    }

    free(mupdf_page);
  }

  return ZATHURA_ERROR_OK;
}

girara_list_t*
pdf_page_search_text(zathura_page_t* page, mupdf_page_t* mupdf_page, const char* text, zathura_error_t* error)
{
  if (page == NULL || text == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL) {
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  if (mupdf_page == NULL || mupdf_page->text == NULL) {
    goto error_ret;
  }

  /* extract text (only once) */
  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document->document, mupdf_page);
  }

  girara_list_t* list = girara_list_new2((girara_free_function_t) zathura_link_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  unsigned int length = text_page_length(mupdf_page->text);
  for (int i = 0; i < length; i++) {
    zathura_rectangle_t* rectangle = g_malloc0(sizeof(zathura_rectangle_t));

    /* search for string */
    int match = text_page_match_string_n(mupdf_page->text, text, i, rectangle);
    if (match == 0) {
      g_free(rectangle);
      continue;
    }

    girara_list_append(list, rectangle);
  }

  return list;

error_free:

  if (list != NULL ) {
    girara_list_free(list);
  }

error_ret:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  return NULL;
}

girara_list_t*
pdf_page_links_get(zathura_page_t* page, mupdf_page_t* mupdf_page, zathura_error_t* error)
{
  if (page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL || mupdf_page == NULL || mupdf_page->page == NULL) {
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);;

  girara_list_t* list = girara_list_new2((girara_free_function_t) zathura_link_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  fz_link* link = pdf_load_links((pdf_document*) mupdf_document->document, mupdf_page->page);
  for (; link != NULL; link = link->next) {
    /* extract position */
    zathura_rectangle_t position;
    position.x1 = link->rect.x0;
    position.x2 = link->rect.x1;
    position.y1 = link->rect.y0;
    position.y2 = link->rect.y1;

    zathura_link_type_t type     = ZATHURA_LINK_INVALID;
    zathura_link_target_t target = { 0 };

    char* buffer = NULL;
    if (link->dest.kind == FZ_LINK_URI) {
      type       = ZATHURA_LINK_EXTERNAL;
      target.uri = g_strdup(link->dest.ld.uri.uri);
    } else if (link->dest.kind == FZ_LINK_GOTO) {
      type               = ZATHURA_LINK_TO_PAGE;
      target.page_number = link->dest.ld.gotor.page;
    } else {
      continue;
    }

    zathura_link_t* zathura_link = zathura_link_new(type, position, target);
    girara_list_append(list, zathura_link);

    if (buffer != NULL) {
      g_free(buffer);
    }
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
pdf_page_get_text(zathura_page_t* page, mupdf_page_t* mupdf_page, zathura_rectangle_t rectangle, zathura_error_t* error)
{
  if (page == NULL || mupdf_page == NULL || mupdf_page->text == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL) {
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  if (mupdf_page == NULL || mupdf_page->text == NULL) {
    goto error_ret;
  }

  /* extract text (only once) */
  if (mupdf_page->extracted_text == false) {
    mupdf_page_extract_text(mupdf_document->document, mupdf_page);
  }

  GString* text           = g_string_new(NULL);
  fz_text_page* text_page = mupdf_page->text;

  for (fz_text_block* block = text_page->blocks; block < text_page->blocks + text_page->len; block++) {
    for (fz_text_line* line = block->lines; line < block->lines + block->len; line++) {
      for (fz_text_span* span = line->spans; span < line->spans + line->len; span++) {
        bool seen = false;

        for (int i = 0; i < span->len; i++) {
          fz_rect hitbox = span->text[i].bbox;
          int c = span->text[i].c;

          if (c < 32) {
            c = '?';
          }

          if (hitbox.x1 >= rectangle.x1
              && hitbox.x0 <= rectangle.x2
              && hitbox.y1 >= rectangle.y1
              && hitbox.y0 <= rectangle.y2) {
            g_string_append_c(text, c);
            seen = true;
          }
        }

        if (seen == true && span + 1 == line->spans + line->len) {
          g_string_append_c(text, '\n');
        }
      }
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

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  return NULL;
}

#if 0
girara_list_t*
pdf_page_images_get(zathura_page_t* page, mupdf_page_t* mupdf_page, zathura_error_t* error)
{
  if (page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  girara_list_t* list = NULL;
  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL) {
    goto error_ret;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  pdf_obj* page_object = pdf_load_object((pdf_document*) mupdf_document->document,zathura_page_get_index(page), 0);
  if (page_object == NULL) {
    goto error_free;
  }

  pdf_obj* resource = pdf_dict_gets(page_object, "Resources");
  if (resource == NULL) {
    goto error_free;
  }

  list = girara_list_new();
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  girara_list_set_free_function(list, (girara_free_function_t) pdf_zathura_image_free);

  get_resources(resource, list);

  return list;

error_free:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  if (list != NULL) {
    girara_list_free(list);
  }

error_ret:

  return NULL;
}

girara_list_t*
pdf_document_get_information(zathura_document_t* document, mupdf_document_t*
    mupdf_document, zathura_error_t* error)
{
  if (document == NULL || mupdf_document == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    return NULL;
  }

  pdf_document* xref = (pdf_document*) mupdf_document->document;
  pdf_obj* object    = pdf_dict_gets(xref->trailer, "Info");
  pdf_obj* info      = pdf_resolve_indirect(object);

  girara_list_t* list = zathura_document_information_entry_list_new();
  if (list == NULL) {
    return NULL;
  }

  for (int i = 0; i < pdf_dict_len(info); i++) {
    pdf_obj* key = pdf_dict_get_key(info, i);
    pdf_obj* val = pdf_dict_get_val(info, i);

    if (pdf_is_name(key) == 0 || pdf_is_string(val) == 0) {
      continue;
    }

    char* name  = pdf_to_name(key);
    char* value = pdf_to_str_buf(val);
    zathura_document_information_type_t type = ZATHURA_DOCUMENT_INFORMATION_OTHER;

    if (strcmp(name, "Author") == 0) {
      type = ZATHURA_DOCUMENT_INFORMATION_AUTHOR;
    } else if (strcmp(name, "Title") == 0) {
      type = ZATHURA_DOCUMENT_INFORMATION_TITLE;
    } else if (strcmp(name, "Subject") == 0) {
      type = ZATHURA_DOCUMENT_INFORMATION_SUBJECT;
    } else if (strcmp(name, "Creator") == 0) {
      type = ZATHURA_DOCUMENT_INFORMATION_CREATOR;
    } else if (strcmp(name, "Producer") == 0) {
      type = ZATHURA_DOCUMENT_INFORMATION_PRODUCER;
    } else if (strcmp(name, "CreationDate") == 0) {
      type = ZATHURA_DOCUMENT_INFORMATION_CREATION_DATE;
    } else if (strcmp(name, "ModDate") == 0) {
      type = ZATHURA_DOCUMENT_INFORMATION_MODIFICATION_DATE;
    }

    zathura_document_information_entry_t* entry = zathura_document_information_entry_new(type, value);
    if (entry != NULL) {
      girara_list_append(list, entry);
    }
  }

  return list;
}
#endif

zathura_image_buffer_t*
pdf_page_render(zathura_page_t* page, mupdf_page_t* mupdf_page, zathura_error_t* error)
{
  if (page == NULL || mupdf_page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    return NULL;
  }

  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL) {
    return NULL;
  }

  /* calculate sizes */
  double scale             = zathura_document_get_scale(document);
  unsigned int page_width  = scale * zathura_page_get_width(page);
  unsigned int page_height = scale * zathura_page_get_height(page);

  /* create image buffer */
  zathura_image_buffer_t* image_buffer = zathura_image_buffer_create(page_width, page_height);

  if (image_buffer == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    return NULL;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  /* render */
  fz_display_list* display_list = fz_new_display_list(mupdf_page->ctx);
  fz_device* device             = fz_new_list_device(mupdf_page->ctx, display_list);

  fz_try (mupdf_document->ctx) {
    pdf_run_page((pdf_document*) mupdf_document->document, mupdf_page->page, device, fz_scale(scale, scale), NULL);
  } fz_catch (mupdf_document->ctx) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    return NULL;
  }

  fz_free_device(device);

  fz_bbox bbox = { .x1 = page_width, .y1 = page_height };

  fz_pixmap* pixmap = fz_new_pixmap_with_bbox(mupdf_page->ctx, fz_device_rgb, bbox);
  fz_clear_pixmap_with_value(mupdf_page->ctx, pixmap, 0xFF);

  device = fz_new_draw_device(mupdf_page->ctx, pixmap);
  fz_run_display_list(display_list, device, fz_identity, bbox, NULL);
  fz_free_device(device);

  unsigned char* s = fz_pixmap_samples(mupdf_page->ctx, pixmap);
  unsigned int n   = fz_pixmap_components(mupdf_page->ctx, pixmap);
  for (unsigned int y = 0; y < fz_pixmap_height(mupdf_page->ctx, pixmap); y++) {
    for (unsigned int x = 0; x < fz_pixmap_width(mupdf_page->ctx, pixmap); x++) {
      guchar* p = image_buffer->data + y * image_buffer->rowstride + x * 3;
      p[0] = s[0];
      p[1] = s[1];
      p[2] = s[2];
      s += n;
    }
  }

  fz_drop_pixmap(mupdf_page->ctx, pixmap);
  fz_free_display_list(mupdf_page->ctx, display_list);

  return image_buffer;
}

#if HAVE_CAIRO
zathura_error_t
pdf_page_render_cairo(zathura_page_t* page, mupdf_page_t* mupdf_page, cairo_t* cairo, bool GIRARA_UNUSED(printing))
{
  if (page == NULL || mupdf_page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  cairo_surface_t* surface = cairo_get_target(cairo);
  if (surface == NULL) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  zathura_document_t* document = zathura_page_get_document(page);
  if (document == NULL) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  mupdf_document_t* mupdf_document = zathura_document_get_data(document);

  unsigned int page_width  = cairo_image_surface_get_width(surface);
  unsigned int page_height = cairo_image_surface_get_height(surface);

  double scalex = ((double) page_width) / zathura_page_get_width(page);
  double scaley = ((double) page_height) /zathura_page_get_height(page);

  /* render */
  fz_display_list* display_list = fz_new_display_list(mupdf_page->ctx);
  fz_device* device             = fz_new_list_device(mupdf_page->ctx, display_list);

  fz_try (mupdf_document->ctx) {
    pdf_run_page((pdf_document*) mupdf_document->document, mupdf_page->page, device, fz_scale(scalex, scaley), NULL);
  } fz_catch (mupdf_document->ctx) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  fz_free_device(device);

  fz_bbox bbox = { .x1 = page_width, .y1 = page_height };

  fz_pixmap* pixmap = fz_new_pixmap_with_bbox(mupdf_page->ctx, fz_device_rgb, bbox);
  fz_clear_pixmap_with_value(mupdf_page->ctx, pixmap, 0xFF);

  device = fz_new_draw_device(mupdf_page->ctx, pixmap);
  fz_run_display_list(display_list, device, fz_identity, bbox, NULL);
  fz_free_device(device);

  int rowstride        = cairo_image_surface_get_stride(surface);
  unsigned char* image = cairo_image_surface_get_data(surface);

  unsigned char* s = fz_pixmap_samples(mupdf_page->ctx, pixmap);
  unsigned int n   = fz_pixmap_components(mupdf_page->ctx, pixmap);
  for (unsigned int y = 0; y < fz_pixmap_height(mupdf_page->ctx, pixmap); y++) {
    for (unsigned int x = 0; x < fz_pixmap_width(mupdf_page->ctx, pixmap); x++) {
      guchar* p = image + y * rowstride + x * 4;
      p[0] = s[0];
      p[1] = s[1];
      p[2] = s[2];
      s += n;
    }
  }

  fz_drop_pixmap(mupdf_page->ctx, pixmap);
  fz_free_display_list(mupdf_page->ctx, display_list);

  return ZATHURA_ERROR_OK;;
}
#endif

static inline int
text_page_char_at(fz_text_page *page, int index)
{
  int offset = 0;
  for (fz_text_block* block = page->blocks; block < page->blocks + page->len; block++) {
    for (fz_text_line* line = block->lines; line < block->lines + block->len; line++) {
      for (fz_text_span* span = line->spans; span < line->spans + line->len; span++) {
        if (index < offset + span->len) {
          return span->text[index - offset].c;
        }

        if (span + 1 == line->spans + line->len) {
          if (index == offset + span->len) {
            return ' ';
          }
          offset++;
        }

        offset += span->len;
      }
    }
  }

  return ' ';
}

static unsigned int
text_page_length(fz_text_page *page)
{
  if (page == NULL) {
    return 0;
  }

  unsigned int length = 0;
  for (fz_text_block* block = page->blocks; block < page->blocks + page->len; block++) {
    for (fz_text_line* line = block->lines; line < block->lines + block->len; line++) {
      for (fz_text_span* span = line->spans; span < line->spans + line->len; span++) {
        length += span->len;
      }

      length++;
    }
  }

  return length;
}

static int
text_page_match_string_n(fz_text_page* page, const char* string, int n,
    zathura_rectangle_t* rectangle)
{
  if (page == NULL || string == NULL || rectangle == NULL) {
    return 0;
  }

  int o = n;
  int c;

  while ((c = *string++)) {
    if (c == ' ' && text_page_char_at(page, n) == ' ') {
      while (text_page_char_at(page, n) == ' ') {
        search_result_add_char(rectangle, page, n);
        n++;
      }
    } else {
      if (tolower(c) != tolower(text_page_char_at(page, n))) {
        return 0;
      }
      search_result_add_char(rectangle, page, n);
      n++;
    }
  }

  return n - o;
}

#if 0
static void
pdf_zathura_image_free(zathura_image_t* image)
{
  if (image == NULL) {
    return;
  }

  g_free(image);
}

static void
get_images(pdf_obj* dict, girara_list_t* list)
{
  if (dict == NULL || list == NULL) {
    return;
  }

  for (int i = 0; i < pdf_dict_len(dict); i++) {
    pdf_obj* image_dict = pdf_dict_get_val(dict, i);
    if (pdf_is_dict(image_dict) == 0) {
      continue;
    }

    pdf_obj* type = pdf_dict_gets(image_dict, "Subtype");
    if (strcmp(pdf_to_name(type), "Image") != 0) {
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

    pdf_obj* width  = pdf_dict_gets(image_dict, "Width");
    pdf_obj* height = pdf_dict_gets(image_dict, "Height");

    zathura_image_t* zathura_image = g_malloc(sizeof(zathura_image_t));

    // FIXME: Get correct image coordinates
    zathura_image->data        = image_dict;
    zathura_image->position.x1 = 0;
    zathura_image->position.x2 = pdf_to_int(width);
    zathura_image->position.y1 = 0;
    zathura_image->position.y2 = pdf_to_int(height);

    girara_list_append(list, zathura_image);
  }
}

static void
get_resources(pdf_obj* resource, girara_list_t* list)
{
  if (resource == NULL || list == NULL) {
    return;
  }

  pdf_obj* x_object = pdf_dict_gets(resource, "XObject");
  if (x_object == NULL) {
    return;
  }

  get_images(x_object, list);

  for (int i = 0; i < pdf_dict_len(x_object); i++) {
    pdf_obj* obj = pdf_dict_get_val(x_object, i);
    pdf_obj* subsrc = pdf_dict_gets(obj, "Resources");
    if (subsrc != NULL && pdf_objcmp(resource, subsrc)) {
      get_resources(subsrc, list);
    }
  }
}
#endif

static void
search_result_add_char(zathura_rectangle_t* rectangle, fz_text_page* page,
    int index)
{
  if (rectangle == NULL || page == NULL) {
    return;
  }

  int offset = 0;
  for (fz_text_block* block = page->blocks; block < page->blocks + page->len; block++) {
    for (fz_text_line* line = block->lines; line < block->lines + block->len; line++) {
      for (fz_text_span* span = line->spans; span < line->spans + line->len; span++) {
        if (index < offset + span->len) {
          fz_rect coordinates = span->text[index - offset].bbox;

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

        if (span + 1 == line->spans + line->len) {
          offset++;
        }

        offset += span->len;
      }
    }
  }
}

static void
mupdf_page_extract_text(fz_document* document, mupdf_page_t* mupdf_page)
{
  if (document == NULL || mupdf_page == NULL || mupdf_page->extracted_text == true) {
    return;
  }

  fz_display_list* display_list = fz_new_display_list(mupdf_page->ctx);
  fz_device* device             = fz_new_list_device(mupdf_page->ctx, display_list);
  fz_device* text_device        = fz_new_text_device(mupdf_page->ctx, mupdf_page->sheet, mupdf_page->text);

  fz_try (mupdf_page->ctx) {
    pdf_run_page((pdf_document*) document, mupdf_page->page, device, fz_identity, NULL);
  } fz_catch (mupdf_page->ctx) {
    goto error_free;
  }

  fz_run_display_list(display_list, text_device, fz_identity, fz_infinite_bbox, NULL);
  mupdf_page->extracted_text = true;

error_free:

  fz_free_device(text_device);
  fz_free_device(device);
  fz_free_display_list(mupdf_page->ctx, display_list);
}

static void
build_index(fz_outline* outline, girara_tree_node_t* root)
{
  if (outline == NULL || root == NULL) {
    return;
  }

  while (outline != NULL) {
    zathura_index_element_t* index_element = zathura_index_element_new(outline->title);

    if (outline->dest.kind == FZ_LINK_URI) {
      index_element->target.uri = g_strdup(outline->dest.ld.uri.uri);
      index_element->type = ZATHURA_LINK_EXTERNAL;
    } else if (outline->dest.kind == FZ_LINK_GOTO) {
      index_element->target.page_number = outline->dest.ld.gotor.page;
      index_element->type = ZATHURA_LINK_TO_PAGE;
    }

    girara_tree_node_t* node = girara_node_append_data(root, index_element);

    if (outline->down != NULL) {
      build_index(outline->down, node);
    }

    outline = outline->next;
  }
}
