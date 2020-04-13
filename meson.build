project('zathura-pdf-mupdf', 'c',
  version: '0.3.5',
  meson_version: '>=0.47',
  default_options: 'c_std=c11'
)

version = meson.project_version()
version_array = version.split('.')

cc = meson.get_compiler('c')

prefix = get_option('prefix')
datadir = get_option('datadir')
metainfodir = join_paths(datadir, 'metainfo')
desktopdir = join_paths(datadir, 'applications')

# required dependencies
libzathura = dependency('libzathura', version: '>=0.0.1', fallback: ['libzathura', 'libzathura_dependency'])
glib = dependency('glib-2.0')
mupdf = cc.find_library('mupdf', version: '>=1.16')
mupdfthird = cc.find_library('mupdf-third')

build_dependencies = [
  libzathura,
  glib,
  mupdf,
  mupdfthird
]

# defines
defines = [
  '-DVERSION_MAJOR=@0@'.format(version_array[0]),
  '-DVERSION_MINOR=@0@'.format(version_array[1]),
  '-DVERSION_REV=@0@'.format(version_array[2]),
  '-D_DEFAULT_SOURCE',
]

if get_option('link-external')
  libjpeg = dependency('libjpeg')
  libjbig2dec = cc.find_library('jbig2dec')
  libopenjp2 = dependency('libopenjp2')

  build_dependencies += [
    libjpeg,
    libjbig2dec,
    libopenjp2
  ]
endif

# compile flags
flags = [
  '-Wall',
  '-Wextra',
  '-pedantic',
  '-Werror=implicit-function-declaration',
  # '-Werror=vla',
]
flags = cc.get_supported_arguments(flags)

# optional dependencies
cairo = dependency('cairo', required: get_option('enable-cairo'))
libfiu = dependency('libfiu', required: get_option('enable-libfiu'))

if cairo.found()
  build_dependencies += cairo
  defines += '-DHAVE_CAIRO'
endif

if libfiu.found()
  build_dependencies += libfiu
  defines += '-DWITH_LIBFIU'
endif

if get_option('plugindir') == ''
  if libzathura.type_name() == 'pkgconfig'
    plugindir = libzathura.get_pkgconfig_variable('plugindir')
  else
    libzathura_subproject = subproject('libzathura')
    plugindir = '/tmp'
  endif
else
  plugindir = get_option('plugindir')
endif

include_directories = [
  include_directories('.')
]

sources = files(
  'zathura-pdf-mupdf/annotations.c',
  'zathura-pdf-mupdf/attachments.c',
  'zathura-pdf-mupdf/document.c',
  'zathura-pdf-mupdf/form-fields.c',
  'zathura-pdf-mupdf/image.c',
  'zathura-pdf-mupdf/links.c',
  'zathura-pdf-mupdf/meta.c',
  'zathura-pdf-mupdf/outline.c',
  'zathura-pdf-mupdf/page.c',
  'zathura-pdf-mupdf/plugin.c',
  'zathura-pdf-mupdf/render.c',
  'zathura-pdf-mupdf/search.c',
  'zathura-pdf-mupdf/select.c',
  'zathura-pdf-mupdf/utils.c'
)

pdf = shared_module('pdf-mupdf',
  sources,
  dependencies: build_dependencies,
  c_args: defines + flags,
  install: true,
  install_dir: plugindir
)

subdir('data')
subdir('tests')
