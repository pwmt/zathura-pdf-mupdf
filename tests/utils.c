/* See LICENSE file for license and copyright information */

#include <check.h>
#include <stdio.h>

#include "utils.h"

const char*
get_plugin_path(void)
{
  return TEST_PLUGIN_FILE_PATH;
}

const char*
get_plugin_dir_path(void)
{
  return TEST_PLUGIN_DIR_PATH;
}

void setup_document_plugin(zathura_plugin_manager_t** plugin_manager, zathura_document_t** document) {
  fail_unless(zathura_plugin_manager_new(plugin_manager) == ZATHURA_ERROR_OK);
  fail_unless(*plugin_manager != NULL);
  fail_unless(zathura_plugin_manager_load(*plugin_manager, get_plugin_path()) == ZATHURA_ERROR_OK);

  zathura_plugin_t* plugin = NULL;
  fail_unless(zathura_plugin_manager_get_plugin(*plugin_manager, &plugin, "application/pdf") == ZATHURA_ERROR_OK);
  fail_unless(plugin != NULL);

  fail_unless(zathura_plugin_open_document(plugin, document, TEST_FILE_PATH, NULL) == ZATHURA_ERROR_OK);
  fail_unless(document != NULL);
}

void setup_document_with_path(zathura_plugin_manager_t** plugin_manager, zathura_document_t** document, const char* path) {
  fail_unless(zathura_plugin_manager_new(plugin_manager) == ZATHURA_ERROR_OK);
  fail_unless(*plugin_manager != NULL);
  fail_unless(zathura_plugin_manager_load(*plugin_manager, get_plugin_path()) == ZATHURA_ERROR_OK);

  zathura_plugin_t* plugin = NULL;
  fail_unless(zathura_plugin_manager_get_plugin(*plugin_manager, &plugin, "application/pdf") == ZATHURA_ERROR_OK);
  fail_unless(plugin != NULL);

  char* complete_path = g_build_path("/", TEST_FILES_PATH, path, NULL);
  fail_unless(zathura_plugin_open_document(plugin, document, complete_path, NULL) == ZATHURA_ERROR_OK);
  g_free(complete_path);
  fail_unless(document != NULL);
}

void setup_document_with_full_path(zathura_plugin_manager_t** plugin_manager, zathura_document_t** document, const char* path) {
  fail_unless(zathura_plugin_manager_new(plugin_manager) == ZATHURA_ERROR_OK);
  fail_unless(*plugin_manager != NULL);
  fail_unless(zathura_plugin_manager_load(*plugin_manager, get_plugin_path()) == ZATHURA_ERROR_OK);

  zathura_plugin_t* plugin = NULL;
  fail_unless(zathura_plugin_manager_get_plugin(*plugin_manager, &plugin, "application/pdf") == ZATHURA_ERROR_OK);
  fail_unless(plugin != NULL);

  fail_unless(zathura_plugin_open_document(plugin, document, path, NULL) == ZATHURA_ERROR_OK);
  fail_unless(document != NULL);
}
