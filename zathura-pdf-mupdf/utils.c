/* See LICENSE file for license and copyright information */

#define _POSIX_C_SOURCE 1

#include "utils.h"

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
