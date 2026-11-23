/* SPDX-License-Identifier: Zlib */

/* Minimal versions of the zathura functions the plugin calls, so that the plugin can be tested without zathura */

#include "zathura-stubs.h"

zathura_document_t* stub_document_new(const char* path) {
  zathura_document_t* document = g_malloc0(sizeof(zathura_document_t));
  document->path               = g_strdup(path);
  return document;
}

void stub_document_free(zathura_document_t* document) {
  for (unsigned int i = 0; i < document->n_pages; ++i) {
    g_free(document->pages[i]);
  }
  g_free(document->pages);
  g_free(document->path);
  g_free(document);
}

zathura_page_t* stub_page_new(zathura_document_t* document, unsigned int index) {
  zathura_page_t* page = g_malloc0(sizeof(zathura_page_t));
  page->document       = document;
  page->index          = index;
  return page;
}

void zathura_annotation_free(zathura_annotation_t* annotation) {
  if (annotation == NULL) {
    return;
  }
  g_free(annotation->quads);
  g_free(annotation->contents);
  g_free(annotation->author);
  g_free(annotation);
}

const char* zathura_document_get_path(zathura_document_t* document) {
  return document->path;
}

const char* zathura_document_get_password(zathura_document_t* GIRARA_UNUSED(document)) {
  return NULL;
}

void* zathura_document_get_data(zathura_document_t* document) {
  return document->data;
}

void zathura_document_set_data(zathura_document_t* document, void* data) {
  document->data = data;
}

void zathura_document_set_number_of_pages(zathura_document_t* document, unsigned int number_of_pages) {
  document->n_pages = number_of_pages;
  document->pages   = g_new0(zathura_page_t*, number_of_pages);
  for (unsigned int i = 0; i < number_of_pages; ++i) {
    document->pages[i] = stub_page_new(document, i);
  }
}

zathura_document_t* zathura_page_get_document(zathura_page_t* page) {
  return page->document;
}

unsigned int zathura_page_get_index(zathura_page_t* page) {
  return page->index;
}

double zathura_page_get_width(zathura_page_t* page) {
  return page->width;
}

double zathura_page_get_height(zathura_page_t* page) {
  return page->height;
}

void zathura_page_set_width(zathura_page_t* page, double width) {
  page->width = width;
}

void zathura_page_set_height(zathura_page_t* page, double height) {
  page->height = height;
}

void zathura_page_set_data(zathura_page_t* page, void* data) {
  page->data = data;
}

/* the plugin only uses these for the index, links and document information, which the tests do not cover */

girara_list_t* zathura_document_information_entry_list_new(void) {
  return girara_list_new();
}

zathura_document_information_entry_t*
zathura_document_information_entry_new(zathura_document_information_type_t GIRARA_UNUSED(type),
                                       const char* GIRARA_UNUSED(value)) {
  return NULL;
}

zathura_index_element_t* zathura_index_element_new(const char* GIRARA_UNUSED(title)) {
  return NULL;
}

zathura_link_t* zathura_link_new(zathura_link_type_t GIRARA_UNUSED(type), zathura_rectangle_t GIRARA_UNUSED(position),
                                 zathura_link_target_t GIRARA_UNUSED(target)) {
  return NULL;
}

void zathura_link_free(zathura_link_t* GIRARA_UNUSED(link)) {}
