# Scene Color

SceneColorView is an optional C++ rendering Gem. It captures HDR scene color immediately before transparent rendering and exposes the snapshot through the shared View SRG. It supplies a Material Canvas node and an AZSL sampling helper without modifying ForwardPassSrg or the engine's pipeline assets.

## Enable and use

1. Enable **Scene Color** in Project Manager and configure/build the project's Editor and launcher. The Gem is available from the engine Gem catalog but is not enabled by default.
2. Close Editor, Material Canvas and Asset Processor before configuration. CMake installs the shared layout described below. Restart the tools after the build and let affected shaders finish processing.
3. Add **Rendering > Scene Color View Capture** to one entity in the level. Choose full, half or quarter width/height and enable capture. Capture is off by default. Half width/height is 25% of full-resolution pixels; quarter is 6.25%.
4. In Material Canvas, use **Scene Color (View SRG)** under View in a transparent material. Leave UV Offset at zero for normal sampling: the node already computes screen UVs. To reproduce the captured color without lighting it a second time, connect RGB to Emissive, set Base Color black and specular/clear coat to zero.

Custom AZSL shaders can include:

```cpp
#include <SceneColorView/SceneColor.azsli>
```

Use `SampleSceneColorView(screenUv)` with normalized screen UVs, or `SampleSceneColorViewFromWorld(positionWS, uvOffset)`. `HasSceneColorView()` reports availability. The helper returns black when the capture is unavailable. Do not directly include the declaration-only `SceneColorView/ViewSrg.azsli` into an individual shader: all consumers must use the shared layout.

## Shared View SRG integration

During CMake configuration the enabled Gem adds a marked, self-contained block to each configured project's `ShaderLib/viewsrg.srgi`. It creates a dated backup, preserves existing project fields, and avoids writing the file again when unchanged. Missing project SRG files are created with the standard partial declaration. Unsupported or edited managed blocks cause configuration to fail instead of being overwritten.

The same declarations must be visible to the runtime `SceneAndViewSrgs` asset and material shaders. A node-only declaration would not produce matching runtime and shader layouts. Installing the extension triggers a broad shader rebuild.

The project block contains declarations, not an include into the Gem. Disabling or physically removing the Gem therefore leaves unused layout fields without breaking unrelated shader includes. It does **not** restore the original SRG layout or promise zero descriptor/constant-layout overhead. No SceneColorView capture pass or capture image is created by the declarations themselves.

Older SceneColorView prototypes installed a marked include. Configure/build once with this version enabled to migrate that include automatically before disabling the Gem. The script only migrates recognized managed blocks.

## Disable or remove

Remove Scene Color components from saved levels and replace/remove materials and graphs that explicitly depend on the Gem. Then disable the Gem in Project Manager, reconfigure/build and restart the tools. The self-contained View SRG extension may remain; no manual include cleanup is required for unrelated shaders.

For standalone distribution, store the Gem outside the project's recursive asset scan directory and register its location. A disabled Gem left inside an ordinary project scan folder can be processed as project assets under a different relative path. The built-in engine location is `Gems/SceneColorView`.

Developers wanting to remove the unused layout extension can use `Tools/integrate_viewsrg.py --project <project> --remove` while the Gem files are still available. This optional operation preserves other project fields and makes a backup, but changes the shared layout and causes shader rebuilding.

## Capture behavior and limitations

- Current-frame sampling is intended for transparent pixel shaders after the capture. This does not let opaque/depth/shadow shaders read a capture produced later in the frame.
- The feature processor inserts the pass before a desktop transparent raster child named `TransparentPass`, under a parent with an `InputOutput` color attachment. Pipelines must permit feature-processor modification; unsupported pipelines are skipped.
- Source images must be resolved, single-sampled, single-layer 2D images. Mobile subpasses, XR/multiview arrays, custom MSAA inputs and cropped/custom viewport mappings are unsupported.
- The snapshot excludes later transparent rendering and subsequent PostFX/UI. Resolution changes resize the persistent capture image at runtime; regenerating a graph is unnecessary.
- Views deliberately shared across pipelines reuse the first capture for that View in the frame. Arbitrary standalone consumer passes still need appropriate frame-graph read dependencies; shader access alone does not schedule or synchronize them.
- Capture settings are scene-wide. Standalone Material Canvas preview scenes are not automatically enabled by a level component.
- Capture disabled skips the capture draw and releases its image. The installed Gem still has feature-processor/pass bookkeeping. GPU timings and broad pipeline/platform regressions have not been measured.

## Validation

User testing on Windows with installed O3DE 2.8.0 confirmed runtime/Editor DLL builds, scene-color rendering in a Material Canvas material, resolution switching, and disable/rebuild without reported errors after moving the standalone Gem outside the project scan root and correcting the old registration path. This is not an automated cross-platform test or a clean test of the final upstream packaging.

Offline integration tests:

```text
python Tools/test_integration.py
```

Standalone image-size assertions, from the Gem root:

```text
g++ -std=c++17 -Wall -Wextra -Werror -ICode/Include Code/Tests/CaptureSizeTests.cpp -o CaptureSizeTests
./CaptureSizeTests
```

See VALIDATION.md for the scope and remaining tests. The code is provided under the MIT license.
