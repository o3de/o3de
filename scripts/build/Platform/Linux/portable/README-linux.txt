O3DE @VERSION@ for Linux (portable)
===================================

This is the O3DE SDK as a plain archive. It contains the same files as the
.deb package, but needs no package manager and no root access: extract it
anywhere you can write to, and delete the folder to remove it.


System requirements
-------------------

  * x86_64 Linux with glibc @MIN_GLIBC@ or newer, and libstdc++ providing
    GLIBCXX_@MIN_GLIBCXX@ or newer (GCC 13 or newer). For example: Ubuntu
    24.04 or newer, Debian 13 or newer, Fedora 43 or newer, openSUSE Leap
    15.6 or newer, RHEL/Alma/Rocky 10, and Arch.
    Ubuntu 22.04 needs a newer libstdc++ first, see "Ubuntu 22.04" below.
    Debian 12 and RHEL/Alma/Rocky 9 are not supported by this build.
  * A GPU and driver with Vulkan support.
  * An X11 or Wayland (XWayland) desktop session.
  * Internet access the first time you run O3DE (it downloads its Python
    runtime and packages into ~/.o3de).
  * Plenty of disk space: the engine, plus the projects you build.


Required system packages
------------------------

O3DE uses some libraries and tools from your distribution. Install them
before the first run. The first group is needed to start O3DE, the second to
build projects.

Ubuntu / Debian:

  # to run
  sudo apt install libglu1-mesa libegl1 libfontconfig1 libxkbcommon0 \
      libxkbcommon-x11-0 libxcb-xinerama0 libxcb-xinput0 libxcb-xfixes0 \
      libxcb-randr0 libxcb-keysyms1 libxcb-cursor0 libpcre2-16-0 zlib1g \
      libunwind8 libzstd1 libcurl4 libtcl8.6 libtk8.6
  # to build projects
  sudo apt install clang ninja-build pkg-config libglu1-mesa-dev \
      mesa-common-dev libfontconfig1-dev libxkbcommon-dev libxkbcommon-x11-dev \
      libxcb-xkb-dev libxcb-xfixes0-dev libxcb-xinput-dev libxcb-randr0-dev \
      libxcb-keysyms1-dev zlib1g-dev libunwind-dev libzstd-dev

Fedora:

  # to run
  sudo dnf install mesa-libGLU mesa-libEGL fontconfig libxkbcommon \
      libxkbcommon-x11 libxcb xcb-util-keysyms xcb-util-cursor pcre2-utf16 \
      zlib libunwind libzstd libcurl tcl tk
  # to build projects
  sudo dnf install clang ninja-build pkgconf-pkg-config mesa-libGLU-devel \
      fontconfig-devel libxkbcommon-devel libxkbcommon-x11-devel libxcb-devel \
      xcb-util-keysyms-devel zlib-devel libunwind-devel libzstd-devel

Arch:

  # to run
  sudo pacman -S --needed glu mesa fontconfig libxkbcommon libxkbcommon-x11 \
      libxcb xcb-util-keysyms xcb-util-cursor pcre2 zlib libunwind zstd curl \
      tcl tk
  # to build projects
  sudo pacman -S --needed clang ninja pkgconf

The Fedora and Arch package names are a guide; if something is still
missing, the launcher lists the exact library files it could not find.

CMake does not need to be installed: a copy is bundled in cmake/runtime.


Ubuntu 22.04
------------

Ubuntu 22.04, and distributions based on it such as Linux Mint 21 and
Pop!_OS 22.04, ship an older libstdc++ than this build needs. Install a
newer one from the Ubuntu toolchain PPA, in addition to the packages above:

  sudo add-apt-repository ppa:ubuntu-toolchain-r/test
  sudo apt update
  sudo apt install libstdc++6 libstdc++-13-dev

This updates the system libstdc++, which stays compatible with existing
programs. libstdc++-13-dev is needed to build projects.


Getting started
---------------

  1. Extract the archive somewhere you own, for example:

       mkdir -p ~/O3DE
       tar -xf @PACKAGE_NAME@.@ARCHIVE_EXT@ -C ~/O3DE

     Do not extract into a system folder such as /opt or /usr/local unless
     you make it writable for your user; O3DE writes to its own folder.

  2. Start the Project Manager:

       ~/O3DE/@TOP_DIR@/o3de

     The first start downloads and sets up O3DE's Python, which takes a few
     minutes. The Project Manager also registers this engine, so you can
     create and open projects straight away.

  3. Optional: add O3DE to your application menu, and the o3de commands to
     ~/.local/bin:

       ~/O3DE/@TOP_DIR@/install-desktop-entry.sh --links

Start O3DE through the Project Manager at least once before using the
command-line tools (scripts/o3de.sh) or launching the Editor directly; the
Project Manager is what sets up Python. Alternatively run
python/get_python.sh yourself.


Updating, moving and removing
-----------------------------

  * Several versions can live side by side, each in its own folder.
  * If you move or rename the folder, start the Project Manager from the new
    location once so the engine is registered there again.
  * To remove O3DE:

       ~/O3DE/@TOP_DIR@/install-desktop-entry.sh --uninstall   # if you used it
       rm -rf ~/O3DE/@TOP_DIR@

    Your projects, settings and downloaded packages in ~/.o3de are kept. The
    engine entry stays listed in ~/.o3de/o3de_manifest.json until you remove
    it from the Project Manager or run:
       scripts/o3de.sh register --remove --this-engine
    before deleting the folder.


Troubleshooting
---------------

"Could not load the Qt platform plugin "xcb""
    A library the xcb plugin needs is missing. Run

      QT_DEBUG_PLUGINS=1 ~/O3DE/@TOP_DIR@/o3de

    and look for the "cannot open shared object file" line, then install the
    package that provides it (on Ubuntu, usually one of the libxcb-* or
    libxkbcommon-x11-0 packages above).

"version `GLIBCXX_3.4.xx' not found"
    Your libstdc++ is older than this build needs. On Ubuntu 22.04, see
    "Ubuntu 22.04" above. Otherwise your distribution is older than this
    build supports (see System requirements).

"version `GLIBC_2.xx' not found"
    Your distribution is older than this build supports (see System
    requirements).

"Failed to start Python"
    The Python download failed. Check your internet connection and run
    python/get_python.sh from the engine folder to see the full error.

Finding every missing library at once:

    find ~/O3DE/@TOP_DIR@/bin/Linux -name '*.so*' -exec ldd {} + 2>/dev/null \
        | grep 'not found' | sort -u


Verifying the download
----------------------

    sha256sum -c @PACKAGE_NAME@.@ARCHIVE_EXT@.sha256
    gpg --verify @PACKAGE_NAME@.@ARCHIVE_EXT@.asc @PACKAGE_NAME@.@ARCHIVE_EXT@


License
-------

O3DE is licensed under Apache-2.0 OR MIT; see LICENSE.txt. Third-party
notices are in NOTICES.txt and SPDX-License.json.
