/* SPDX-License-Identifier: Zlib */

#include "utils.h"

void mupdf_page_extract_text(mupdf_document_t* mupdf_document, mupdf_page_t* mupdf_page) {
  if (mupdf_document == NULL || mupdf_document->ctx == NULL || mupdf_page == NULL || mupdf_page->text == NULL) {
    return;
  }

  fz_device* volatile text_device = NULL;

  fz_try(mupdf_page->ctx) {
    fz_stext_options stext_options;
    stext_options.flags = FZ_STEXT_PRESERVE_IMAGES;
    text_device = fz_new_stext_device(mupdf_page->ctx, mupdf_page->text, &stext_options);

    fz_run_page(mupdf_page->ctx, mupdf_page->page, text_device, fz_identity, NULL);
  }
  fz_always(mupdf_document->ctx) {
    fz_close_device(mupdf_page->ctx, text_device);
    fz_drop_device(mupdf_page->ctx, text_device);
  }
  fz_catch(mupdf_document->ctx) {}

  mupdf_page->extracted_text = true;
}

char* read_xdg_config_file(const char* filename) {
    // Get XDG_CONFIG_HOME, default to ~/.config if not set
    const char* config_home = getenv("XDG_CONFIG_HOME");
    char* config_dir;

    if (config_home && config_home[0] != '\0') {
        config_dir = strdup(config_home);
    } else {
        const char* home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "Cannot determine home directory\n");
            return NULL;
        }
        // Allocate space for HOME + "/.config" + null terminator
        config_dir = malloc(strlen(home) + 9);
        sprintf(config_dir, "%s/.config", home);
    }

    // Construct full path
    char* filepath = malloc(strlen(config_dir) + strlen(filename) + 2);
    sprintf(filepath, "%s/%s", config_dir, filename);
    free(config_dir);

    // Open file
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        perror("Failed to open file");
        free(filepath);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer (+1 for null terminator)
    char* content = malloc(size + 1);
    if (!content) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        free(filepath);
        return NULL;
    }

    // Read file
    size_t bytes_read = fread(content, 1, size, file);
    content[bytes_read] = '\0';  // Null terminate

    fclose(file);
    free(filepath);

    return content;
}
