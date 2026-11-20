O3DE offline installer
======================

This zip contains the complete O3DE installer, so installing needs no
internet connection for the engine files.

  1. Unblock the zip before extracting it, so Windows does not flag the
     installer files. In PowerShell:

       Unblock-File .\@OFFLINE_NAME@.zip

  2. Extract the whole zip. Keep all files in the same folder: the installer
     reads the .msi and .cab files from next to it.

  3. Run @BOOTSTRAPPER@ and follow the steps.

The first time you start O3DE, it still downloads its Python runtime and
packages into %USERPROFILE%\.o3de, so that step needs internet access.

To use O3DE without installing it, download the portable zip
(@PACKAGE_NAME@_windows_portable.zip) instead.
