/* SPDX-License-Identifier: Zlib */

#ifndef ZATHURA_STUBS_H
#define ZATHURA_STUBS_H

#include <glib.h>
#include <zathura/plugin-api.h>

struct zathura_document_s {
  void* data;
  char* path;
  unsigned int n_pages;
  zathura_page_t** pages;
};

struct zathura_page_s {
  zathura_document_t* document;
  unsigned int index;
  double width;
  double height;
  void* data;
};

zathura_document_t* stub_document_new(const char* path);
void stub_document_free(zathura_document_t* document);
zathura_page_t* stub_page_new(zathura_document_t* document, unsigned int index);

#endif
