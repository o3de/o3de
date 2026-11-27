# LYShineToShine — Canvas Upgrader Gem

This Gem migrates UI content authored with the legacy **LyShine** UI system (slice-based) to the
**Shine** UI system (prefab-based). It is a transitional tool: enable it, convert your content,
then disable it.

## What it converts

| Input | Output |
|---|---|
| `.uicanvas` (v1/v2/v3, slice-based, XML `SliceComponent` hierarchy) | `.uicanvas` v4 (flat `ChildEntities` list, prefab-instance metadata) |
| `.slice` files containing UI entities | `.uiprefab` (JSON prefab format used by Shine) |

Non-UI slices are detected and skipped.

## Prerequisites

- The **Shine** Gem must be enabled in your project (it replaces LyShine).
- Your original LyShine `.slice` assets must **still exist on disk**. The canvas upgrader
  instantiates slice references in order to flatten them into the canvas, so do not delete
  or move your old slice files until after the upgrade completes.
- Run the upgrade from a build that includes this Gem (enable `LYShineToShine` in your
  project's `project.json` or via the Project Manager).

## Upgrade workflow

Run the two console commands from the Editor console, **in this order**:

1. **Convert slices to UI prefabs** — do this first so canvases upgraded in step 2 can
   reference the new `.uiprefab` assets:

   ```
   convert_slices <directory_path>
   ```

   Recursively scans `<directory_path>` for `.slice` files containing UI entities and writes a
   `.uiprefab` next to each one. Non-UI slices are skipped and reported.

2. **Upgrade canvases**:

   ```
   upgrade_canvases <directory_path>
   ```

   Recursively scans `<directory_path>` for `.uicanvas` files and upgrades any that are in an
   old format. Canvases already in v4 format are left untouched. Slice instances embedded in a
   canvas are flattened into plain child entities; references to converted slices are remapped
   to the corresponding `.uiprefab`.

Each command prints a report (files scanned / converted / skipped / failed) with per-file
failure details.

3. **Verify** — open the upgraded canvases in the UI Editor and confirm they load and render
   correctly. Let the Asset Processor finish reprocessing the changed assets.

4. **Clean up** — once everything is verified:
   - Delete the old `.slice` source files that were converted.
   - Disable the `LYShineToShine` Gem in your project.

## Notes

- The upgrader modifies `.uicanvas` files **in place**. Use source control (or a backup) so
  you can diff and revert if needed.
- Paths passed to the commands are absolute paths or paths relative to the current working
  directory of the Editor process.
- The LyShine and LyShineExamples Gems remain in the engine source tree during the deprecation
  period but are unregistered; projects should migrate to Shine using this Gem.
