# Linux portable tarball

`make_portable_tarball.sh` creates a portable O3DE archive that runs without a package manager or root access. It takes an O3DE `.deb` or an installed engine folder and writes:

- `o3de_<version>_linux_<arch>.tar.xz`, or `.tar.gz` with `-z`
- a `.sha256` checksum file
- a detached `.asc` GPG signature, with `-s <key id>`

The archive has the same files as the `.deb`, with the post-install steps already applied: the bundled CMake is extracted and build leftovers are removed. The script also adds these files from this folder:

| File | Purpose |
| --- | --- |
| `o3de` | Starts the Project Manager and lists any missing system libraries |
| `install-desktop-entry.sh` | Optional application menu entry and `~/.local/bin` links for the current user |
| `README-linux.txt` | User instructions. The script fills in the version and the minimum glibc and libstdc++ versions |

## Requirements

- `tar`, plus `xz` or `gzip` (`pigz` is used if it is installed)
- `dpkg-deb`, or `ar` from binutils on systems without dpkg
- `objdump` from binutils, to find the minimum glibc and libstdc++ versions. Without it, the README lists them as unknown
- `gpg`, only when signing

## Examples

From build output (CPack):

```sh
./make_portable_tarball.sh -o dist build/linux/CPackUploads/o3de_26.10.0.deb
```

From a prebuilt `.deb`, compressed as `.tar.gz`:

```sh
./make_portable_tarball.sh -z -o dist ~/Downloads/o3de_26.10.0.deb
```

From an installed `.deb`:

```sh
./make_portable_tarball.sh -o dist /opt/O3DE/26.10.0
```

Run `./make_portable_tarball.sh --help` for all options.
