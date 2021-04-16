# Cauldron

A simple framework for rapid prototyping on Vulkan or DirectX12.

Cauldron was developed by AMD and is used internally to develop prototypes, demos and SDK samples.

Cauldron is compiled as a static library, hence the sample application has to statically link against it.

# Sample Projects & Screenshots

- [glTFSample](https://github.com/GPUOpen-LibrariesAndSDKs/glTFSample), a simple demo app to show off Cauldron's features
  ![Screenshot](https://github.com/GPUOpen-LibrariesAndSDKs/glTFSample/raw/master/screenshot.png)

# Cauldron Features

- [glTF 2.0](https://github.com/KhronosGroup/glTF/tree/master/specification/2.0) File loader
  - Animation for cameras, objects, skeletons and lights
  - Skinning
  - PBR Materials 
    - Metallic-Roughness 
    - Specular-Glossiness (KHR_materials_pbrSpecularGlossiness)
  - Lighting (KHR_lights_punctual extension)
    - Spot Lights w/ Shadows (up to 4)
    - Image-based Lighting (IBL) CubeMaps
- Postprocessing
  - Bloom
  - HDR/Tonemapping
- Texture Loaders for DDS, JPEG and PNG formats
  - MipMap generation for powers-of-two textures
- In-app user interface using [Dear ImGui](https://github.com/ocornut/imgui)
- Rendering Resource Management
  - Command Buffer Ring
  - Timestamp Queries
  - Memory Heap Manager (Linear Allocator)
  - Static buffers for VB/IB/CB with suballocation support
  - Dynamic buffers for VB/IB/CB using a ring buffer
- Debug Rendering
  - Wireframe Bounding Boxes
  - Light Frustums
- Window management & swapchain creation
  - Fullscreen/Windowed Modes
- Support for DXC/SM6.x 
- Shader Binary & PSO caching

# Directory Structure

- `build` : Build scripts & generated solution files/folders
- `libs` : Libraries
  - `AGS` : AMD helper library for querying GPU info
  - `VulkanMemoryAllocator` : Helper library for memory management with Vulkan applications
  - `d3d12x` : The D3D12 helper library
  - `dxc` : DirectX Shader Compiler 
  - `imgui` : Graphical User Interface library
  - `json` : Library for adding JSON support w/ C++ syntax
- `media` : Builtin textures and other data
- `src` : Source code files
  - [`common`](./src/common/) : Common code used by both DX12/VK
  - [`DX12`](./src/DX12/) : DirectX12 backend
  - [`VK`](./src/VK/) : Vulkan backend

Note: more info on the rendering backends can be found in the Readme of their respective folders.

# Build

## Prerequisites

- [CMake 3.4](https://cmake.org/download/)
- [Visual Studio 2017](https://visualstudio.microsoft.com/downloads/)
- [Windows 10 SDK 10.0.17763.0](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk)
- [Vulkan SDK 1.1.106](https://www.lunarg.com/vulkan-sdk/)

## How-to-Build

- Run the `GenerateSolutions.bat` file in the `build` directory.
- `build/VK` and `build/DX12` folders will contain the `Framework_*.sln` files
- Simply build the solution file for the desired API
- `build/DX12/src/DX12/` directory will contain the compiled static library `Framework_DX12.lib` for DX12 (similar for VK) under the selected configuration (Debug/Release) folder.

# Framework Architecture

Every feature in Cauldron has been implemented in a single file using C++11.

The main features could be grouped in 4 categories:

- **Resource managers & loaders**
  - Texture loaders - *can load `DDS`, `PNG`, `JPG` and any other file supported by the WIC subsystem. It can also create image views.*
  - StaticBufferPool - *holds vertices and indices for static geometry. It creates a huge buffer and then suballocates chunks from it in a linear fashion. Chunks cannot be freed.*
  - DynamicBufferRing - *holds constant buffers and also vertices and indices. Same as before but allocation happens in a ring fashion.*
  - ResourceViewHeaps - *holds the Descriptor Sets.*  
  - UploadHeap - *system memory buffer that holds the texture data to be uploaded to video memory via DMA*
  - CommandListRing - *allocates a number of command buffers from a ring buffer and returns a ready-to-use command buffer.*
  - Device - *all the device related code.*
- **Renderers**
  - GLTF* - *loads and renders the model*
  - PostProcPS/PS - *draws a 2D quad using a custom shader, used by the post processing passes.*
  - ImGUI - *code for driving the ImGUI*
  - Widgets
    - Wireframe - *draws a wireframe cube (used for rendering bounding boxes and light/camera frustums)*
    - Axis - *draws the coordinate axis of a matrix*
- **Vulkan specific helpers & setup code**
  - InstanceVK - *creates an instance and enables the validation layer*
  - DebugMarkersExtVk - *sets up the debug markers*
- **Commons & OS-specific code**
  - Swapchain - *handles resizing, fullscreen/windowed modes, etc.*
  - FrameworkWindows - *creates a window, processes windows message pump*

Cauldron was originally written using DX12 and later on ported to Vulkan using the same structure. This would make Cauldron ideal for learning Vulkan if you're coming from DirectX12, or vice versa.

# Known Issues

Please bear in mind that in order to keep Cauldron simple we are only covering the most frequently used features (for now). 

Please feel free to [open an issue](TODO:LinkToGitHubRepoIssuesPage) for bug reports.

# Contribution

Cauldron should be very easy to extend, should you want to contribute to Cauldron, you can [open a merge request](TODO:LinkToGitHubRepoMergeRequestPage).

# 3rd-Party Open Source Projects

- [ImGUI](https://github.com/ocornut/imgui)
- [json](https://github.com/nlohmann/json)
- [AMD GPU Services (AGS) SDK](https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [DirectX Shader Compiler](https://github.com/Microsoft/DirectXShaderCompiler)
- [D3DX12](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Libraries/D3DX12)
