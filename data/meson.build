install_data('org.pwmt.zathura-pdf-mupdf.metainfo.xml', install_dir: metainfodir)

conf_data = configuration_data()
pdf = ''
if get_option('pdf').enabled()
  conf_data.set(
    'PDF',
    'application/pdf;'
  )
else
  conf_data.set('PDF', '')
endif

desktop = configure_file(
  input: join_paths(meson.current_source_dir(), 'org.pwmt.zathura-pdf-mupdf.desktop'),
  output: 'org.pwmt.zathura-pdf-mupdf.desktop',
  configuration: conf_data,
)

install_data(desktop, install_dir: desktopdir)

desktop_file_validate = find_program('desktop-file-validate', required: get_option('tests'), native: true)
if desktop_file_validate.found()
  test('validate-desktop',
    desktop_file_validate,
    args: [desktop]
  )
endif

appstream_util = find_program('appstreamcli', required: get_option('tests'), native: true)
if appstream_util.found()
  test('validate-appdata',
    appstream_util,
    args: ['validate', '--no-net', join_paths(meson.current_source_dir(), 'org.pwmt.zathura-pdf-mupdf.metainfo.xml')]
  )
endif
