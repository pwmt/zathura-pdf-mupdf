zathura-pdf-mupdf
=================

zathura is a highly customizable and functional document viewer based on the girara user interface
library and several document libraries. This plugin for zathura provides PDF support using the
`mupdf` library.

Requirements
------------

The following dependencies are required:

* `zathura` (>= 0.2.0)
* `girara`
* `mupdf` (>= 1.25.0)

For building plugin, the following dependencies are also required:

* `meson` (>= 1)

Installation
------------

To build and install the plugin using meson's ninja backend:

    meson build
    cd build
    ninja
    ninja install

> **Note:** The default backend for meson might vary based on the platform. Please
refer to the meson documentation for platform specific dependencies.

> **Note:** To avoid conflicts with `zathura-pdf-poppler`, PDF support can be disabled
at compile time by using `meson build -Dpdf=disabled` instead of `meson build`.

Bugs
----

Please report bugs at https://github.com/pwmt/zathura-pdf-mupdf.
