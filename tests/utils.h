/* See LICENSE file for license and copyright information */

#include <libzathura/libzathura.h>
#include <libzathura/plugin-manager.h>

#define _STR(x) #x
#define STR(x) _STR(x)
#define TEST_PLUGIN_DIR_PATH STR(_TEST_PLUGIN_DIR_PATH)
#define TEST_PLUGIN_FILE_PATH STR(_TEST_PLUGIN_FILE_PATH)
#define TEST_FILE_PATH STR(_TEST_FILE_PATH)
#define TEST_FILES_PATH STR(_TEST_FILES_PATH)

const char* get_plugin_path(void);
const char* get_plugin_dir_path(void);
void setup_document_plugin(zathura_plugin_manager_t** plugin_manager, zathura_document_t** document);
void setup_document_with_path(zathura_plugin_manager_t** plugin_manager, zathura_document_t** document, const char* path);
void setup_document_with_full_path(zathura_plugin_manager_t** plugin_manager, zathura_document_t** document, const char* path);
