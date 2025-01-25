install_data('org.pwmt.zathura-pdf-mupdf.metainfo.xml', install_dir: metainfodir)
install_data('org.pwmt.zathura-pdf-mupdf.desktop', install_dir: desktopdir)

conf_data = configuration_data()
pdf = ''
if get_option('pdf')
  pdf = 'application/pdf;'
endif
conf_data.set(
  'PDF',
  pdf,
  description: 'mimetype for pdf',
)
configure_file(
  input: join_paths(meson.current_source_dir(), 'org.pwmt.zathura-pdf-mupdf.desktop'),
  output: 'org.pwmt.zathura-pdf-mupdf.desktop',
  configuration: conf_data,
)

desktop_file_validate = find_program('desktop-file-validate', required: get_option('tests'), native: true)
if desktop_file_validate.found()
  test('validate-desktop',
    desktop_file_validate,
    args: ['org.pwmt.zathura-pdf-mupdf.desktop']
  )
endif

appstream_util = find_program('appstream-util', required: get_option('tests'), native: true)
if appstream_util.found()
  test('validate-appdata',
    appstream_util,
    args: ['validate-relax', '--nonet', join_paths(meson.current_source_dir(), 'org.pwmt.zathura-pdf-mupdf.metainfo.xml')]
  )
endif
