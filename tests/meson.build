check = dependency('check', required: get_option('tests'))
if check.found()
  test_dependencies = [
    declare_dependency(link_with: pdf),
    glib,
    check
  ]

  # default timeout
  default_timeout = 60*60

  # Set plugin path for tests
  defines += [
    '-D_TEST_PLUGIN_DIR_PATH=@0@/@1@'.format(meson.project_build_root(), pdf.outdir()),
    '-D_TEST_PLUGIN_FILE_PATH=@0@'.format(pdf.full_path()),
    '-D_TEST_FILE_PATH=@0@'.format(meson.current_source_dir() + '/files/empty.pdf'),
    '-D_TEST_FILES_PATH=@0@'.format(meson.current_source_dir()),
  ]

  if get_option('enable-libfiu') and libfiu.found()
    fiu_run = find_program('fiu-run')
  else
    fiu_run = disabler()
  endif

  components = {
    'annotations': ['annotations.c'],
    'attachments': ['attachments.c'],
    'document': ['document.c'],
    'form-fields': ['form-fields.c'],
    'image': ['image.c'],
    'links': ['links.c'],
    'meta': ['meta.c'],
    'outline': ['outline.c'],
    'page': ['page.c'],
    'render': ['render.c'],
    'search': ['search.c'],
    'select': ['select.c'],
  }

  foreach name, sources: components
    exec = executable('test-' + name,
      sources + ['utils.c', 'tests.c'],
      dependencies: build_dependencies + test_dependencies,
      include_directories: include_directories,
      c_args: defines + flags
    )

    if fiu_run.found() and libfiu.found()
      test(name, fiu_run,
        args: ['-x', exec],
        timeout: default_timeout
      )
    else
      test(name, exec,
        timeout: default_timeout
      )
    endif
  endforeach
endif
