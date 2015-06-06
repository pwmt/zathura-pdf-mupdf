/* See LICENSE file for license and copyright information */

#include <libzathura/libzathura.h>
#include <libzathura/plugin-manager.h>

const char* get_plugin_path(void);
void setup_document_with_path(zathura_plugin_manager_t** plugin_manager, zathura_document_t** document, const char* path);
