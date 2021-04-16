The VK backend
================

Here lives all the VK specific code. It should be as similar as possible to the DX12 directory.

### Directory structure

* **base**: All the basic code to manage and load textures/buffers/command buffers
* **GLTF**: Code to render the scene using different techniques
    * GLTFBBoxPass: renders just the bounding boxes of the objects
    * GLTFBDepthPass: creates optimized geometry, descriptor sets and pipelines for a depth pass. It also renders the pass and supports skinning.
    * GLTFPbrPass: same as above but for the forward PBR pass. (Credits go to the glTF-WebGL-PBR github project for its fantastic PBR shader.)
    * TexturesAndBuffers: It loads the texture and buffers from disk and uploads into video memory it also uploads the skinning matrices buffer.
* **PostProc**
    * PostProcCS/PostProcPS: that takes a shader and some inputs and draws a full screen quad (used by the effects below)
    * Bloom: combines the DownsamplePS and the Blur passes to create a Bloom effect. (All the credits go to Jorge Jimenez!)
    * BlurPS: takes a mip chain and blurs every mip element
    * DownSamplePS: takes a render target and generates a mip chain    
    * SkyDome: loads a diffuse+LUT and a specular skydome cubemap. This is used for Image Based Lighting (IBL) and also for drawing a skydome.
    * SkydomeProc: draws a skydome procedurally using the Preetham Model. (Credits go to Simon Wallner and Martin Upitis.) TODO: Generate a diffuse cubemap for IBL.
    * ToneMapping: implements a number of tonemapping methods.
* **Shaders**
    * all the shaders used by the above classes
* **Widgets**
    * Axis: Renders an axis
    * WireFrameBox: Renders a box in wireframe
    * WireFrameSphere: Renders a sphere in wireframe
    * CheckerBoardFloor: as stated.

Note: All the code that may be common with DX12 is in [src/common](src/common)
