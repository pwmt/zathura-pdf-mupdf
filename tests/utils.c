/* See LICENSE file for license and copyright information */

#include <check.h>
#include <stdio.h>

#include "utils.h"

const char*
get_plugin_path(void)
{
#ifdef BUILD_DEBUG
  return "../build/debug/pdf.so";
#elif BUILD_GCOV
  return "../build/gcov/pdf.so";
#else
  return "../build/release/pdf.so";
#endif
}

void setup_document_with_path(zathura_plugin_manager_t** plugin_manager, zathura_document_t** document, const char* path) {
  fail_unless(zathura_plugin_manager_new(plugin_manager) == ZATHURA_ERROR_OK);
  fail_unless(*plugin_manager != NULL);
  fail_unless(zathura_plugin_manager_load(*plugin_manager, get_plugin_path()) == ZATHURA_ERROR_OK);

  zathura_plugin_t* plugin = NULL;
  fail_unless(zathura_plugin_manager_get_plugin(*plugin_manager, &plugin, "application/pdf") == ZATHURA_ERROR_OK);
  fail_unless(plugin != NULL);

  fail_unless(zathura_plugin_open_document(plugin, document, path, NULL) == ZATHURA_ERROR_OK);
  fail_unless(document != NULL);
}

