O3DE @VERSION@ for Windows (portable)
=====================================

This is the O3DE SDK as a plain zip file. It contains the same files as the
installer, but needs no installation and no administrator rights: extract it
anywhere you can write to, and delete the folder to remove it.


System requirements
-------------------

  * Windows 10 or 11, 64-bit.
  * A GPU and driver with DirectX 12 or Vulkan support.
  * Microsoft Visual C++ Redistributable for Visual Studio 2015-2022 (x64).
    Most systems already have it; if O3DE reports that VCRUNTIME140.dll or
    MSVCP140.dll is missing, install it from Microsoft:
      https://aka.ms/vs/17/release/vc_redist.x64.exe
  * To build projects: Visual Studio 2022 with the "Desktop development with
    C++" workload (or "Game development with C++"), including a Windows 10 or
    11 SDK.
  * Internet access the first time you run O3DE (it downloads its Python
    runtime and packages into %USERPROFILE%\.o3de).
  * Plenty of disk space: the engine, plus the projects you build.

CMake does not need to be installed: a copy is bundled in cmake\runtime.


Getting started
---------------

  1. Unblock the zip before extracting it. Windows marks every file
     extracted from a downloaded zip as "from the internet", which causes
     security prompts and can stop scripts from running. In PowerShell:

       Unblock-File .\@PACKAGE_NAME@.zip

     or right-click the zip, choose Properties, tick "Unblock", and click OK.

  2. Extract it to a short path that you own, for example C:\O3DE. Some
     engine paths are long, and deep folders can exceed Windows' path length
     limit. Avoid C:\Program Files, which is not writable for normal users;
     O3DE writes to its own folder.

     Extracting with Explorer is slow for an archive this size. These are
     faster:

       tar -xf @PACKAGE_NAME@.zip -C C:\O3DE
       7z x @PACKAGE_NAME@.zip -oC:\O3DE

  3. Start the Project Manager by double-clicking:

       C:\O3DE\@TOP_DIR@\o3de.cmd

     The first start downloads and sets up O3DE's Python, which takes a few
     minutes. The Project Manager also registers this engine, so you can
     create and open projects straight away.

  4. Optional: add O3DE to the Start menu (and the desktop) by running:

       C:\O3DE\@TOP_DIR@\create-shortcuts.cmd -Desktop

Start O3DE through the Project Manager at least once before using the
command-line tools (scripts\o3de.bat) or launching the Editor directly; the
Project Manager is what sets up Python. Alternatively run
python\get_python.bat yourself.


Updating, moving and removing
-----------------------------

  * Several versions can live side by side, each in its own folder, and next
    to an installer-based install.
  * If you move or rename the folder, start the Project Manager from the new
    location once so the engine is registered there again, and run
    create-shortcuts.cmd again if you use the shortcuts.
  * To remove O3DE:

       C:\O3DE\@TOP_DIR@\create-shortcuts.cmd -Uninstall   (if you used it)
       C:\O3DE\@TOP_DIR@\scripts\o3de.bat register --remove --this-engine
       rmdir /s /q C:\O3DE\@TOP_DIR@

    Your projects, settings and downloaded packages in %USERPROFILE%\.o3de
    are kept. Without the "register --remove" step, the engine stays listed
    in %USERPROFILE%\.o3de\o3de_manifest.json until you remove it from the
    Project Manager.


Troubleshooting
---------------

"Windows protected your PC" or scripts that refuse to run
    The files still carry the "from the internet" mark. Unblock them all:

      Get-ChildItem -Recurse C:\O3DE\@TOP_DIR@ | Unblock-File

"VCRUNTIME140.dll was not found" or "MSVCP140.dll was not found"
    Install the Visual C++ Redistributable (see System requirements).

"Failed to start Python"
    The Python download failed. Check your internet connection and run
    python\get_python.bat from the engine folder to see the full error.

Files are missing after extracting, or extraction fails part way
    The path is too long. Extract to a shorter folder such as C:\O3DE, or
    enable long paths in Windows (Group Policy: Computer Configuration >
    Administrative Templates > System > Filesystem > Enable Win32 long paths).


Verifying the download
----------------------

    In PowerShell, compare the output with the contents of
    @PACKAGE_NAME@.zip.sha256:

      (Get-FileHash .\@PACKAGE_NAME@.zip -Algorithm SHA256).Hash

    With GnuPG installed:

      gpg --verify @PACKAGE_NAME@.zip.asc @PACKAGE_NAME@.zip


License
-------

O3DE is licensed under Apache-2.0 OR MIT; see LICENSE.txt. Third-party
notices are in NOTICES.txt and SPDX-License.json.
