install_data('org.pwmt.zathura-pdf-mupdf.metainfo.xml', install_dir: metainfodir)
install_data('org.pwmt.zathura-pdf-mupdf.desktop', install_dir: desktopdir)

desktop_file_validate = find_program('desktop-file-validate', required: get_option('tests'), native: true)
if desktop_file_validate.found()
  test('validate-desktop',
    desktop_file_validate,
    args: [join_paths(meson.current_source_dir(), 'org.pwmt.zathura-pdf-mupdf.desktop')]
  )
endif

appstream_util = find_program('appstream-util', required: get_option('tests'), native: true)
if appstream_util.found()
  test('validate-appdata',
    appstream_util,
    args: ['validate-relax', '--nonet', join_paths(meson.current_source_dir(), 'org.pwmt.zathura-pdf-mupdf.metainfo.xml')]
  )
endif
