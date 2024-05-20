project('zathura-pdf-mupdf', 'c',
  version: '0.4.2',
  meson_version: '>=0.61',
  default_options: ['c_std=c17', 'warning_level=3']
)

version = meson.project_version()
version_array = version.split('.')

cc = meson.get_compiler('c')

prefix = get_option('prefix')
datadir = get_option('datadir')
metainfodir = join_paths(datadir, 'metainfo')
desktopdir = join_paths(datadir, 'applications')

# required dependencies
zathura = dependency('zathura', version: '>=0.5.2')
girara = dependency('girara-gtk3')
glib = dependency('glib-2.0')
cairo = dependency('cairo')
mupdf = dependency('mupdf', required: false, version: '>=1.24')
mupdfthird = cc.find_library('mupdf-third')

build_dependencies = [
  zathura,
  girara,
  glib,
  cairo,
]

if not mupdf.found()
  # normal build of mupdf
  mupdf = cc.find_library('mupdf')
  cc.check_header('mupdf/fitz/version.h', required: true)
  if not cc.compiles('
#include <mupdf/fitz/version.h>

#if FZ_VERSION_MAJOR < 1 || (FZ_VERSION_MAJOR == 1 && FZ_VERSION_MINOR < 24)
#error "mupdf 1.24 or newer is requried"
#endif
')
    error('mupdf 1.24 or newer is required')
  endif

  build_dependencies += [mupdf, mupdfthird]
else
  # build from Debian's libmupdf-dev
  build_dependencies += [mupdf, mupdfthird]

  libjpeg = dependency('libjpeg')
  libjbig2dec = cc.find_library('jbig2dec')
  libopenjp2 = dependency('libopenjp2')
  gumbo = dependency('gumbo')
  tesseract = dependency('tesseract')
  leptonica = dependency('lept')
  mujs = dependency('mujs')

  build_dependencies += [
    libjpeg,
    libjbig2dec,
    libopenjp2,
    gumbo,
    tesseract,
    leptonica,
    mujs
  ]
endif

if get_option('plugindir') == ''
  plugindir = zathura.get_variable(pkgconfig: 'plugindir')
else
  plugindir = get_option('plugindir')
endif

# defines
defines = [
  '-DVERSION_MAJOR=@0@'.format(version_array[0]),
  '-DVERSION_MINOR=@0@'.format(version_array[1]),
  '-DVERSION_REV=@0@'.format(version_array[2]),
  '-D_DEFAULT_SOURCE',
]

# compile flags
flags = [
  '-Werror=implicit-function-declaration',
  '-Werror=vla',
]
flags = cc.get_supported_arguments(flags)

sources = files(
  'zathura-pdf-mupdf/document.c',
  'zathura-pdf-mupdf/image.c',
  'zathura-pdf-mupdf/index.c',
  'zathura-pdf-mupdf/links.c',
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
  install_dir: plugindir,
  gnu_symbol_visibility: 'hidden'
)

subdir('data')
