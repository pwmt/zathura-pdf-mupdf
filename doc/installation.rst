Installation
============

Dependencies
------------

The core of zathura-pdf-mupdf depends on `libzathura <http://pwmt.org>`_ and
`mupdf <http://mupdf.com>`_. In addition we require some
libraries for testing and building the documentation. You can find a full list
of the dependencies here:

* `glib <http://gnome.org>`_ >= 2.32.4
* `check <http://check.sorceforge.net>`_ >= 0.9.8
* `libfiu <http://blitiri.com.ar/p/libfiu>`_ >= 0.91
* `python-docutils <http://docutils.sourceforge.net>`_
* `python-sphinx <http://sphinx-doc.org>`_

Stable version
--------------

.. note::

Since zathura-pdf-mupdf packages are available for many distributions it is
recommended to use them by installing them with your prefered package manager.
Otherwise you can grab the latest version of the source code from our website
and build it by hand:

.. code-block:: sh

  tar xfv zathura-pdf-mupdf-<version>.tar.gz
  cd zathura-pdf-mupdf-<version>
  make
  make install

Developer version
-----------------

If you are interested in testing the very latest versions with all its new
features, that we are working on, type in the following commands:

.. code-block:: sh

  git clone git://pwmt.org/zathura-pdf-mupdf.git
  cd zathura-pdf-mupdf
  git checkout --track -b develop origin/develop
  make
  make install
