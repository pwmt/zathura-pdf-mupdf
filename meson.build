project('zathura-pdf-mupdf', 'c',
  version: '0.3.4',
  meson_version: '>=0.43',
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
zathura = dependency('zathura', version: '>=0.3.9')
girara = dependency('girara-gtk3')
glib = dependency('glib-2.0')
cairo = dependency('cairo')
mupdf = cc.find_library('mupdf')
mupdfthird = cc.find_library('mupdf-third')
libjpeg = dependency('libjpeg')
libjbig2dec = cc.find_library('jbig2dec')
libopenjp2 = dependency('libopenjp2')

build_dependencies = [
  zathura,
  girara,
  glib,
  cairo,
  mupdf,
  mupdfthird,
  libjpeg,
  libjbig2dec,
  libopenjp2

]


if get_option('plugindir') == ''
  plugindir = zathura.get_pkgconfig_variable('plugindir')
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
  '-Wall',
  '-Wextra',
  '-pedantic',
  '-Werror=implicit-function-declaration',
  '-Werror=vla',
  '-fvisibility=hidden'
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
  install_dir: plugindir
)

subdir('data')
