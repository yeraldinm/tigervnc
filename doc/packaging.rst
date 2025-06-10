Cross-Platform Packaging
=======================

TigerVNC provides several helper scripts for creating packages on various
platforms.  The ``release`` directory includes build targets for macOS and
Windows, and binary tarballs for generic Unix systems.  Distribution specific
recipes for RPM and DEB based systems are available under ``contrib/packages``.

Snapcraft
---------

A basic ``snapcraft.yaml`` is provided under ``packaging/snap``.  To build
a Snap package run::

    snapcraft --destructive-mode

The resulting ``*.snap`` file can be installed locally using ``sudo snap install``
or uploaded to the Snap store.

Flatpak
-------

``packaging/flatpak`` contains a Flatpak manifest for building the viewer.
Create the Flatpak bundle with::

    flatpak-builder --force-clean build-dir \
      packaging/flatpak/com.tigervnc.vncviewer.yaml

The produced ``tigervnc.flatpak`` can then be installed or distributed.

macOS and Windows
-----------------

The existing CMake build offers ``make dmg`` and ``make installer`` targets to
create a macOS disk image or Windows installer.  See ``BUILDING.txt`` for more
information.
