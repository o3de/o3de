# Windows offline and portable zips

These scripts create two packages from the O3DE installer:

- **Offline installer zip** (`o3de_<version>_windows_offline.zip`): the bootstrapper, `.msi` and `.cab` files, so installing needs no internet access for the engine files.
- **Portable zip** (`o3de_<version>_windows_portable.zip`): the engine files as the `.msi` installs them, so O3DE runs without installation or administrator rights. The bundled CMake is already extracted and build leftovers are removed.

Each zip gets a `.sha256` checksum file and, with `-GpgKey <key id>`, a detached `.asc` GPG signature.

| File | Purpose |
| --- | --- |
| `make_windows_packages.ps1` | Creates both zips from the bootstrapper `.exe`, or from a folder with the `.msi` and `.cab` files |
| `make_portable_zip.ps1` | Creates only the portable zip, from an `.msi` or an installed engine folder |
| `package_helpers.ps1` | Shared functions for the two scripts above |
| `o3de.cmd` | Added to the portable zip. Starts the Project Manager and shows a message until its window opens |
| `create-shortcuts.cmd`, `create-shortcuts.ps1` | Added to the portable zip. Optional Start menu and desktop shortcuts for the current user |
| `README-windows.txt` | Added to the portable zip. User instructions |
| `README-offline.txt` | Added to the offline zip. User instructions |

## Requirements

- Windows 10 1803 or newer, for `tar.exe`
- 7-Zip, optional but much faster than `tar.exe` for archives this size
- `gpg`, only when signing

Keep the output path short. Staging happens inside it, and some engine paths are long.

## Examples

From build output (CPack), both zips:

```powershell
.\make_windows_packages.ps1 -OutputDir C:\dist build\windows\CPackUploads\o3de_26.10.0_installer.exe
```

From a folder with the prebuilt `.msi` and `.cab` files, both zips:

```powershell
.\make_windows_packages.ps1 -OutputDir C:\dist -LayoutDir D:\o3de_layout
```

Only the portable zip, from the `.msi`. The `.cab` files must be next to it:

```powershell
.\make_portable_zip.ps1 -OutputDir C:\dist D:\o3de_layout\o3de_26.10.0.msi
```

Only the portable zip, from an installed engine:

```powershell
.\make_portable_zip.ps1 -OutputDir C:\dist "C:\Program Files\O3DE\26.10.0"
```

Use `-SkipOffline` or `-SkipPortable` with `make_windows_packages.ps1` to create only one of the zips. Run `Get-Help .\make_windows_packages.ps1 -Detailed` for all options.
