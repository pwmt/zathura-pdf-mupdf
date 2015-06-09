/* See LICENSE file for license and copyright information */

#include <stdio.h>
#include <stdlib.h>

#define _POSIX_C_SOURCE 1

#include "plugin.h"
#include "internal.h"

static zathura_error_t mupdf_form_field_to_zathura_form_field(zathura_page_t*
    page, mupdf_document_t* mupdf_document, pdf_widget* widget,
    zathura_form_field_t** form_field);

typedef struct mupdf_form_field_s {
  pdf_widget* widget;
  mupdf_document_t* document;
} mupdf_form_field_t;

zathura_error_t
pdf_page_get_form_fields(zathura_page_t* page, zathura_list_t** form_fields)
{
  if (page == NULL || form_fields == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;
  *form_fields = NULL;

  zathura_document_t* document;
  if (zathura_page_get_document(page, &document) != ZATHURA_ERROR_OK || document == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_out;
  }

  mupdf_document_t* mupdf_document;
  if ((error = zathura_document_get_data(document, (void**) &mupdf_document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_page_t* mupdf_page;
  if ((error = zathura_page_get_data(page, (void**) &mupdf_page)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  pdf_widget* widget = pdf_first_widget(mupdf_document->ctx, (pdf_document*) mupdf_document->document, (pdf_page*) mupdf_page->page);
  while (widget != NULL) {
    zathura_form_field_mapping_t* mapping = calloc(1, sizeof(zathura_form_field_mapping_t));
    if (mapping == NULL) {
      goto error_free;
    }

    zathura_form_field_t* form_field;
    if (mupdf_form_field_to_zathura_form_field(page, mupdf_document, widget, &form_field) !=
        ZATHURA_ERROR_OK) {
      widget = pdf_next_widget(mupdf_document->ctx, widget);
      continue;
    }

    fz_rect bounding_box;
    pdf_bound_widget(mupdf_document->ctx, widget, &bounding_box);

    zathura_rectangle_t position = {
      {bounding_box.x0, bounding_box.y0},
      {bounding_box.x1, bounding_box.y1}
    };

    mapping->position   = position;
    mapping->form_field = form_field;

    *form_fields = zathura_list_append(*form_fields, mapping);

    /* next widget */
    widget = pdf_next_widget(mupdf_document->ctx, widget);
  }

  return error;

error_free:

  zathura_list_free_full(*form_fields, free);
  *form_fields = NULL;

error_out:

  return error;
}

static zathura_error_t
mupdf_form_field_to_zathura_form_field(zathura_page_t* page, mupdf_document_t*
    mupdf_document, pdf_widget* widget, zathura_form_field_t** form_field)
{
  zathura_error_t error = ZATHURA_ERROR_OK;
  zathura_form_field_type_t zathura_type = ZATHURA_FORM_FIELD_UNKNOWN;

  int widget_type = pdf_widget_get_type(mupdf_document->ctx, widget);

  switch (widget_type) {
    case PDF_WIDGET_TYPE_NOT_WIDGET:
      zathura_type = ZATHURA_FORM_FIELD_UNKNOWN;
      break;
    case PDF_WIDGET_TYPE_PUSHBUTTON:
    case PDF_WIDGET_TYPE_RADIOBUTTON:
    case PDF_WIDGET_TYPE_CHECKBOX:
      zathura_type = ZATHURA_FORM_FIELD_BUTTON;
      break;
    case PDF_WIDGET_TYPE_TEXT:
      zathura_type = ZATHURA_FORM_FIELD_TEXT;
      break;
    case PDF_WIDGET_TYPE_LISTBOX:
    case PDF_WIDGET_TYPE_COMBOBOX:
      zathura_type = ZATHURA_FORM_FIELD_CHOICE;
      break;
    case PDF_WIDGET_TYPE_SIGNATURE:
      zathura_type = ZATHURA_FORM_FIELD_SIGNATURE;
      break;
  }

  /* create new form field */
  if ((error = zathura_form_field_new(page, form_field, zathura_type)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  /* set user data */
  mupdf_form_field_t* mupdf_form_field = calloc(1, sizeof(mupdf_form_field_t));
  if (mupdf_form_field == NULL) {
    goto error_out;
  }

  mupdf_form_field->widget   = widget;
  mupdf_form_field->document = mupdf_document;

  if ((error = zathura_form_field_set_user_data(*form_field, mupdf_form_field, free)) != ZATHURA_ERROR_OK) {
    goto error_free;
  }

  // TODO: Get name, partial name and mapping name

  pdf_annot* annot = (pdf_annot*) widget;
  int field_flags = pdf_get_field_flags(mupdf_document->ctx, (pdf_document*) mupdf_document->document, annot->obj);

  /* set general properties */
  gboolean is_read_only = (field_flags & Ff_ReadOnly) ? true : false;
  if (is_read_only == TRUE && (error =
        zathura_form_field_set_flags(*form_field, ZATHURA_FORM_FIELD_FLAG_READ_ONLY)) !=
      ZATHURA_ERROR_OK) {
    goto error_free;
  }

  switch (zathura_type) {
    case ZATHURA_FORM_FIELD_UNKNOWN:
      break;
    /* button field */
    case ZATHURA_FORM_FIELD_BUTTON:
      {
        zathura_form_field_button_type_t button_type = ZATHURA_FORM_FIELD_BUTTON_TYPE_PUSH;

        switch (widget_type) {
          case PDF_WIDGET_TYPE_PUSHBUTTON:
            button_type = ZATHURA_FORM_FIELD_BUTTON_TYPE_PUSH;
            break;
          case PDF_WIDGET_TYPE_RADIOBUTTON:
            button_type = ZATHURA_FORM_FIELD_BUTTON_TYPE_RADIO;
            break;
          case PDF_WIDGET_TYPE_CHECKBOX:
            button_type = ZATHURA_FORM_FIELD_BUTTON_TYPE_CHECK;
            break;
        }

        if ((error = zathura_form_field_button_set_type(*form_field, button_type)) != ZATHURA_ERROR_OK) {
          goto error_free;
        }

        bool state = false;

        pdf_obj* as = pdf_dict_get(mupdf_document->ctx, annot->obj, PDF_NAME_AS);
        if (as != NULL && strcmp(pdf_to_name(mupdf_document->ctx, as), "Yes") == 0) {
          state = true;
        }

        if ((error = zathura_form_field_button_set_state(*form_field, state)) !=
            ZATHURA_ERROR_OK) {
          goto error_free;
        }
      }
      break;
    /* text field */
    case ZATHURA_FORM_FIELD_TEXT:
      {
        zathura_form_field_text_type_t text_type = ZATHURA_FORM_FIELD_TEXT_TYPE_NORMAL;

        if (field_flags & Ff_Multiline) {
          text_type = ZATHURA_FORM_FIELD_TEXT_TYPE_MULTILINE;
        } else if (field_flags & Ff_FileSelect) {
          text_type = ZATHURA_FORM_FIELD_TEXT_TYPE_FILE_SELECT;
        } else {
          text_type = ZATHURA_FORM_FIELD_TEXT_TYPE_NORMAL;
        }

        if ((error = zathura_form_field_text_set_type(*form_field, text_type)) != ZATHURA_ERROR_OK) {
          goto error_free;
        }

        unsigned int max_length = pdf_text_widget_max_len(mupdf_document->ctx, (pdf_document*) mupdf_document->document, widget);
        if ((error = zathura_form_field_text_set_max_length(*form_field, max_length)) !=
            ZATHURA_ERROR_OK) {
          goto error_free;
        }

        gchar* text = pdf_text_widget_text(mupdf_document->ctx, (pdf_document*) mupdf_document->document, widget);
        if (text != NULL && (error =
              zathura_form_field_text_set_text(*form_field, text)) !=
            ZATHURA_ERROR_OK) {
          goto error_free;
        }

        bool do_scroll = (field_flags & Ff_DoNotScroll) ? false : true;
        if ((error = zathura_form_field_text_set_scroll(*form_field, do_scroll)) !=
            ZATHURA_ERROR_OK) {
          goto error_free;
        }

        bool do_spell_check = (field_flags & Ff_DoNotSpellCheck) ? false : true;
        if ((error = zathura_form_field_text_set_spell_check(*form_field, do_spell_check)) !=
            ZATHURA_ERROR_OK) {
          goto error_free;
        }

        bool is_password = (field_flags & Ff_Password) ? true : false;
        if ((error = zathura_form_field_text_set_password(*form_field, is_password)) !=
            ZATHURA_ERROR_OK) {
          goto error_free;
        }

        bool is_rich_text = (field_flags & Ff_RichText) ? true : false;
        if ((error = zathura_form_field_text_set_rich_text(*form_field, is_rich_text)) !=
            ZATHURA_ERROR_OK) {
          goto error_free;
        }
      }
      break;
    /* choice field */
    case ZATHURA_FORM_FIELD_CHOICE:
      {
        zathura_form_field_choice_type_t choice_type = ZATHURA_FORM_FIELD_CHOICE_TYPE_COMBO;

        switch (widget_type) {
          case PDF_WIDGET_TYPE_LISTBOX:
            choice_type = ZATHURA_FORM_FIELD_CHOICE_TYPE_LIST;
          case PDF_WIDGET_TYPE_COMBOBOX:
            choice_type = ZATHURA_FORM_FIELD_CHOICE_TYPE_COMBO;
            break;
        }

        if ((error = zathura_form_field_choice_set_type(*form_field, choice_type)) != ZATHURA_ERROR_OK) {
          goto error_free;
        }

        bool can_select_multiple = (field_flags & Ff_MultiSelect) ? true : false;
        if ((error = zathura_form_field_choice_set_multiselect(*form_field, can_select_multiple)) !=
            ZATHURA_ERROR_OK) {
          goto error_free;
        }

        bool is_editable = (field_flags & Ff_Edit) ? true : false;
        if ((error = zathura_form_field_choice_set_editable(*form_field, is_editable)) !=
            ZATHURA_ERROR_OK) {
          goto error_free;
        }

        bool do_spell_check = (field_flags & Ff_DoNotSpellCheck) ? false : true;
        if ((error = zathura_form_field_choice_set_spell_check(*form_field, do_spell_check)) !=
            ZATHURA_ERROR_OK) {
          goto error_free;
        }

        unsigned int number_of_items =
          pdf_choice_widget_options(mupdf_document->ctx, (pdf_document*)
              mupdf_document->document, widget, NULL);
        unsigned int number_of_values =
          pdf_choice_widget_value(mupdf_document->ctx, (pdf_document*)
              mupdf_document->document, widget, NULL);

        if (number_of_items > 0) {
          char* options[number_of_items];
          pdf_choice_widget_options(mupdf_document->ctx, (pdf_document*) mupdf_document->document, widget, options);
          char* values[number_of_values];
          pdf_choice_widget_value(mupdf_document->ctx, (pdf_document*) mupdf_document->document, widget, values);

          for (unsigned int i = 0; i < number_of_items; i++) {
            char* name = options[i];

            zathura_form_field_choice_item_t* item;
            if ((error = zathura_form_field_choice_item_new(*form_field, &item, name)) != ZATHURA_ERROR_OK) {
              goto error_free;
            }

            for (unsigned int j = 0; j < number_of_values; j++) {
              if (strcmp(name, values[j]) == 0 && (error = zathura_form_field_choice_item_select(item)) !=
                  ZATHURA_ERROR_OK) {
                goto error_free;
              }
            }
          }
        }
      }
      break;
    /* signature field */
    case ZATHURA_FORM_FIELD_SIGNATURE:
      {
        error = ZATHURA_ERROR_PLUGIN_NOT_IMPLEMENTED;
      }
      break;
  }

  return error;

error_free:

  free(mupdf_form_field);

error_out:

  return error;
}

zathura_error_t
pdf_form_field_save(zathura_form_field_t* form_field)
{
  if (form_field == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_form_field_type_t type;
  if (zathura_form_field_get_type(form_field, &type) != ZATHURA_ERROR_OK) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  mupdf_form_field_t* mupdf_form_field;
  if (zathura_form_field_get_user_data(form_field, (void**) &mupdf_form_field) != ZATHURA_ERROR_OK) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  mupdf_document_t* mupdf_document = mupdf_form_field->document;
  pdf_widget* widget = mupdf_form_field->widget;

  switch (type) {
    case ZATHURA_FORM_FIELD_UNKNOWN:
      return ZATHURA_ERROR_INVALID_ARGUMENTS;
    case ZATHURA_FORM_FIELD_BUTTON:
      {
        zathura_form_field_button_type_t button_type;
        if (zathura_form_field_button_get_type(form_field, &button_type) !=
            ZATHURA_ERROR_OK) {
          return ZATHURA_ERROR_INVALID_ARGUMENTS;
        }

        bool state;
        if (zathura_form_field_button_get_state(form_field, &state) != ZATHURA_ERROR_OK) {
          return ZATHURA_ERROR_INVALID_ARGUMENTS;
        }

        pdf_annot* annot = (pdf_annot*) widget;
        pdf_obj* value = pdf_new_name(mupdf_document->ctx, (pdf_document*) mupdf_document->document, state ? "Yes" : "Off");
        pdf_dict_put(mupdf_document->ctx, annot->obj, PDF_NAME_AS, value);
      }
      break;
    case ZATHURA_FORM_FIELD_TEXT:
      {
        char* text;
        if (zathura_form_field_text_get_text(form_field, &text) != ZATHURA_ERROR_OK) {
          return ZATHURA_ERROR_INVALID_ARGUMENTS;
        }

        pdf_text_widget_set_text(mupdf_document->ctx, (pdf_document*) mupdf_document->document, widget, text);
      }
      break;
    case ZATHURA_FORM_FIELD_CHOICE:
      {
        zathura_list_t* choice_items;
        if (zathura_form_field_choice_get_items(form_field, &choice_items) != ZATHURA_ERROR_OK) {
          return ZATHURA_ERROR_INVALID_ARGUMENTS;
        }

        /* get mupdf options */
        unsigned int number_of_items =
          pdf_choice_widget_options(mupdf_document->ctx, (pdf_document*)
              mupdf_document->document, widget, NULL);

        char* options[number_of_items];
        pdf_choice_widget_options(mupdf_document->ctx, (pdf_document*) mupdf_document->document, widget, options);

        char** values = NULL;
        unsigned int number_of_values = 0;

        /* iterate over all items */
        zathura_form_field_choice_item_t* choice_item;
        ZATHURA_LIST_FOREACH(choice_item, choice_items) {
          bool is_selected;
          if (zathura_form_field_choice_item_is_selected(choice_item, &is_selected) != ZATHURA_ERROR_OK) {
            continue;
          }

          if (is_selected == true) {
            char* name;
            if (zathura_form_field_choice_item_get_name(choice_item, &name) != ZATHURA_ERROR_OK) {
              continue;
            }

            char** tmp = realloc(values, sizeof(char*) * (number_of_values + 1));
            if (tmp != NULL) {
              values = tmp;
              values[number_of_values++] = name;
            }
          }
        }

        if (number_of_values > 0) {
          pdf_choice_widget_set_value(mupdf_document->ctx, (pdf_document*) mupdf_document->document, widget, number_of_values, values);
          free(values);
        }
      }
      break;
    case ZATHURA_FORM_FIELD_SIGNATURE:
      break;
  }

  return ZATHURA_ERROR_OK;
}
