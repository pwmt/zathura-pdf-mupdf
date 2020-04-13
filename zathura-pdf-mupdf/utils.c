/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include <string.h>
#include <libzathura/libzathura.h>

#include "utils.h"
#include "macros.h"

void
mupdf_page_extract_text(mupdf_document_t* mupdf_document, mupdf_page_t* mupdf_page) {
  if (mupdf_document == NULL || mupdf_document->ctx == NULL || mupdf_page == NULL ||
      mupdf_page->text == NULL) {
    return;
  }

  fz_device* volatile text_device = NULL;

  fz_try (mupdf_page->ctx) {
    text_device = fz_new_stext_device(mupdf_page->ctx, mupdf_page->text, NULL);

    /* Disable FZ_DONT_INTERPOLATE_IMAGES to collect image blocks */
    fz_disable_device_hints(mupdf_page->ctx, text_device, FZ_DONT_INTERPOLATE_IMAGES);

    fz_matrix ctm = fz_scale(1.0, 1.0);
    fz_run_page(mupdf_page->ctx, mupdf_page->page, text_device, ctm, NULL);
  } fz_always (mupdf_document->ctx) {
    fz_close_device(mupdf_page->ctx, text_device);
    fz_drop_device(mupdf_page->ctx, text_device);
  } fz_catch(mupdf_document->ctx) {
  }

  mupdf_page->extracted_text = true;
}

bool
mupdf_link_to_zathura_action(fz_context* ctx, const char* uri, zathura_action_t** action)
{
  if (uri == NULL || action == NULL) {
    return false;
  }

  if (uri == NULL) {
    if (zathura_action_new(action, ZATHURA_ACTION_NONE) != ZATHURA_ERROR_OK) {
      return false;
    }

    return true;
  } else if (fz_is_external_link(ctx, uri) == 1) {
    if (strstr(uri, "file://") == uri) {
      if (zathura_action_new(action, ZATHURA_ACTION_GOTO_REMOTE) != ZATHURA_ERROR_OK) {
        return false;
      }

      return true;
    } else {
      if (zathura_action_new(action, ZATHURA_ACTION_URI) != ZATHURA_ERROR_OK) {
        return false;
      }

      return true;
    }
  } else {
    if (zathura_action_new(action, ZATHURA_ACTION_GOTO) != ZATHURA_ERROR_OK) {
      return false;
    }

    return true;
  }

  if (zathura_action_new(action, ZATHURA_ACTION_UNKNOWN) != ZATHURA_ERROR_OK) {
    return false;
  }

  // FIXME: Support the following
#if 0
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
  }
#endif

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

zathura_annotation_border_t
mupdf_border_to_zathura_border(fz_context* ctx, pdf_obj* annotation)
{
  /* Initialize border with default values */
  zathura_annotation_border_t border;
  zathura_annotation_border_init(&border);

  /* Parse border style */
  pdf_obj* dict = pdf_dict_get(ctx, annotation, PDF_NAME(BS));
  pdf_obj* obj = NULL;

  if (pdf_is_dict(ctx, dict) != 0) {
    /* Parse width */
    obj = pdf_dict_get(ctx, dict, PDF_NAME(W));
    if (pdf_is_number(ctx, obj)) {
      border.width = pdf_to_int(ctx, obj);
    }

    /* Parse style */
    typedef struct annotation_border_style_mapping_s {
      const char* name;
      zathura_annotation_border_style_t style;
    } annotation_border_style_mapping_t;

    annotation_border_style_mapping_t border_style_mapping[] = {
      { "S", ZATHURA_ANNOTATION_BORDER_STYLE_SOLID },
      { "D", ZATHURA_ANNOTATION_BORDER_STYLE_DASHED },
      { "B", ZATHURA_ANNOTATION_BORDER_STYLE_BEVELED },
      { "I", ZATHURA_ANNOTATION_BORDER_STYLE_INSET },
      { "U", ZATHURA_ANNOTATION_BORDER_STYLE_UNDERLINE },
    };

    obj = pdf_dict_get(ctx, dict, PDF_NAME(S));
    if (pdf_is_name(ctx, obj)) {
      const char* name = pdf_to_name(ctx, obj);
      for (unsigned int i = 0; i < LENGTH(border_style_mapping); i++) {
        if (strncmp(name, border_style_mapping[i].name, 1) == 0) {
          border.style = border_style_mapping[i].style;
        }
      }
    }

    /* Parse dash array */
    obj = pdf_dict_get(ctx, dict, PDF_NAME(D));
    if (pdf_is_array(ctx, obj)) {
      zathura_list_t* dash_array = NULL;

      int length = pdf_array_len(ctx, obj);
      for (int i = 0; i < length; i++) {
        pdf_obj* entry = pdf_array_get(ctx, obj, i);
        if (pdf_is_number(ctx, entry)) {
          int number = pdf_to_int(ctx, entry);
          dash_array = zathura_list_append(dash_array, (gpointer) (size_t) number);
        }
      }

      border.dash_pattern.dash_array = dash_array;
      border.dash_pattern.dash_phase = 0;
    }
  }

  /* Parse border effect */
  dict = pdf_dict_get(ctx, annotation, PDF_NAME(BE));

  if (pdf_is_dict(ctx, dict) != 0) {
    /* Parse intensity */
    obj = pdf_dict_get(ctx, dict, PDF_NAME(I));
    if (pdf_is_number(ctx, obj)) {
      border.intensity = pdf_to_int(ctx, obj);
    }

    /* Parse style */
    obj = pdf_dict_get(ctx, dict, PDF_NAME(S));
    if (pdf_is_name(ctx, obj)) {
      const char* name = pdf_to_name(ctx, obj);
      if (strncmp(name, "C", 1) == 0) {
        border.effect = ZATHURA_ANNOTATION_BORDER_EFFECT_CLOUDY;
      } else {
        border.effect = ZATHURA_ANNOTATION_BORDER_EFFECT_NONE;
      }
    }
  }

  return border;
}
