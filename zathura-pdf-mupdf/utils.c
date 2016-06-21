/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include <string.h>

#include "utils.h"
#include "macros.h"

void
mupdf_page_extract_text(mupdf_document_t* mupdf_document, mupdf_page_t* mupdf_page) {
  if (mupdf_document == NULL || mupdf_document->ctx == NULL || mupdf_page == NULL ||
      mupdf_page->sheet == NULL || mupdf_page->text == NULL) {
    return;
  }

  fz_device* text_device = NULL;

  fz_try (mupdf_page->ctx) {
    text_device = fz_new_stext_device(mupdf_page->ctx, mupdf_page->sheet, mupdf_page->text);

    /* Disable FZ_IGNORE_IMAGE to collect image blocks */
    fz_disable_device_hints(mupdf_page->ctx, text_device, FZ_IGNORE_IMAGE);

    fz_matrix ctm;
    fz_scale(&ctm, 1.0, 1.0);
    fz_run_page(mupdf_page->ctx, mupdf_page->page, text_device, &ctm, NULL);
  } fz_always (mupdf_document->ctx) {
    fz_drop_device(mupdf_page->ctx, text_device);
  } fz_catch(mupdf_document->ctx) {
  }

  mupdf_page->extracted_text = true;
}

bool
mupdf_to_zathura_action(fz_link_dest* link, zathura_action_t** action)
{
  if (link == NULL || action == NULL) {
    return false;
  }

  switch (link->kind) {
    case FZ_LINK_NONE:
      if (zathura_action_new(action, ZATHURA_ACTION_NONE) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    case FZ_LINK_URI:
      if (zathura_action_new(action, ZATHURA_ACTION_URI) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    case FZ_LINK_GOTO:
      if (zathura_action_new(action, ZATHURA_ACTION_GOTO) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    case FZ_LINK_LAUNCH:
      if (zathura_action_new(action, ZATHURA_ACTION_LAUNCH) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    case FZ_LINK_NAMED:
      if (zathura_action_new(action, ZATHURA_ACTION_NAMED) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    case FZ_LINK_GOTOR:
      if (zathura_action_new(action, ZATHURA_ACTION_GOTO_REMOTE) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    default:
      if (zathura_action_new(action, ZATHURA_ACTION_UNKNOWN) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
  }

  return true;
}

typedef struct blend_mode_map_s {
  const char* string;
  zathura_blend_mode_t blend_mode;
} blend_mode_map_t;

static blend_mode_map_t blend_mode_mapping[] = {
  { "Normal",     ZATHURA_BLEND_MODE_NORMAL },
  { "Multiply",   ZATHURA_BLEND_MODE_MULTIPLY },
  { "Screen",     ZATHURA_BLEND_MODE_SCREEN },
  { "Overlay",    ZATHURA_BLEND_MODE_OVERLAY },
  { "Darken",     ZATHURA_BLEND_MODE_DARKEN },
  { "Lighten",    ZATHURA_BLEND_MODE_LIGHTEN },
  { "ColorDodge", ZATHURA_BLEND_MODE_COLOR_DODGE },
  { "ColorBurn",  ZATHURA_BLEND_MODE_COLOR_BURN },
  { "HardLight",  ZATHURA_BLEND_MODE_HARD_LIGHT },
  { "SoftLight",  ZATHURA_BLEND_MODE_SOFT_LIGHT },
  { "Difference", ZATHURA_BLEND_MODE_DIFFERENCE },
  { "Exclusion",  ZATHURA_BLEND_MODE_EXCLUSION },
};

zathura_blend_mode_t
mupdf_blend_mode_to_zathura_blend_mode(const char* blend_mode_str)
{
  if (blend_mode_str == NULL) {
    return ZATHURA_BLEND_MODE_NORMAL;
  }

  for (unsigned int i = 0; i < LENGTH(blend_mode_mapping); i++) {
    if (strcmp(blend_mode_str, blend_mode_mapping[i].string) == 0) {
      return blend_mode_mapping[i].blend_mode;
    }
  }

  return ZATHURA_BLEND_MODE_NORMAL;
}

const char*
zathura_blend_mode_to_mupdf_blend_mode(zathura_blend_mode_t blend_mode)
{
  for (unsigned int i = 0; i < LENGTH(blend_mode_mapping); i++) {
    if (blend_mode_mapping[i].blend_mode == blend_mode) {
      return blend_mode_mapping[i].string;
    }
  }

  return "Normal";
}

zathura_annotation_color_t
mupdf_color_to_zathura_color(fz_context* ctx, pdf_obj* obj)
{
  zathura_annotation_color_t color = {
    ZATHURA_ANNOTATION_COLOR_SPACE_RGB,
    {0}
  };

  if (pdf_is_array(ctx, obj)) {
    unsigned int n = pdf_array_len(ctx, obj);
    for (unsigned int i = 0; i < n; i++) {
      pdf_obj* item = pdf_array_get(ctx, obj, i);

      if (pdf_is_number(ctx, item)) {
        float number = pdf_to_real(ctx, item);
        color.values[i] = number * 65535.0;
      }
    }
  }

  return color;
}
