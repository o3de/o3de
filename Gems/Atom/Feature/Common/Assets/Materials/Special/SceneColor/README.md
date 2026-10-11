# Standalone Scene Color material test

Assign `SceneColorDistortionTest.material` to a plane in a well-lit Main Pipeline scene. The image behind the plane should show waves. Set Distortion Strength to 0 to compare with the undistorted image; the default is 0.025.

The shader includes `SceneColorSrg.azsli`, not `ForwardPassSrg.azsli`. Its `sceneColor` draw list is consumed by `SceneColorPass`, which binds the existing pre-transparency snapshot. It does not create another capture.

The shader writes unlit HDR color, tests scene depth and does not write depth. The captured image excludes transparent objects and later post-processing. Material Editor previews without this pipeline connection may not display the effect.

Local testing: the user confirmed the original consumer rendered successfully before its general naming update. The test shader compiled locally for DX12 and Vulkan. This repository packaging changes asset locations and has only static validation so far. Mobile and MultiView are unsupported.
