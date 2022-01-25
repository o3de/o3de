# AMD TressFX

![AMD TressFX](http://gpuopen.com/GitHub_Media/TressFX4.1-wideshot1.jpg)

The TressFX library is AMD's hair/fur rendering and simulation technology. TressFX is designed to use the GPU to simulate and render high-quality, realistic hair and fur. 
The TressFX/Radeon&reg; Cauldron sample is built on top of Radeon&reg; Cauldron, which is an open sourced framework used at AMD for sample creation. It supports both DirectX&reg; 12 and Vulkan&reg; through a custom wrapper layer. For more information about Radeon&reg; Cauldron, or to download the source/examples, please refer to https://gpuopen.com/gamingproduct/caudron-framework/.

With this release we aim to make it easier for developers to integrate TressFX into an existing codebase.
Additionally, TressFX 4.1 provides some performance and feature updates compared to TressFX 4.0 including faster Velocity Shock Propagation, simplified Local Shape Constraints, a reorganization of dispatches, new rendering features, extensive documentation and tutorials as well as an updated TressFX Exporter for Autodesk&reg; Maya&reg;.

This release also demonstrates TressFX integration with Epic Games Unreal Engine (4.22), a minimal integration but with multiple TressFX components, features, and rendering and simulation materials to help ease-of-use within Unreal engine.  Developers wishing to further the integration or customize it for their own requirements should find this basic level of integration a helpful first step in that process.

<div>
  <a href="https://github.com/GPUOpen-Effects/TressFX/releases/latest/"><img src="http://gpuopen-effects.github.io/media/latest-release-button.svg" alt="Latest release" title="Latest release"></a>
</div>

### Highlights include the following:
* Designed for optimized rendering and simulation
* Hair and fur support, with high quality anti-aliasing
* Animation/skinning support
* Epic Games Unreal 4.22 engine integration (engine patch available)
* TressFX/Radeon&reg; Cauldron implementation 
* Autodesk&reg; Maya&reg; plugin provided for hair/fur and collision authoring
* Full source code provided

![AMD TressFX](http://gpuopen.com/GitHub_Media/TressFX4.1-wideshot2.jpg)

### New in TressFX 4.1
* TressFX/Radeon&reg; Cauldron implementation with full source code (DirectX&reg; 12 and Vulkan&reg;)
* Optimized physics simulation shaders allowing more hair to be simulated in real-time
  * faster Velocity Shock Propagation, simplified Local Shape Constraints, reorganization of dispatches
* New rendering features
  * StrandUV and Hair Parameter Blending*
* New Level of Detail (LOD) system
* Extensive documentation and tutorials
* Updated Autodesk&reg; Maya&reg; Exporter with new UI and new features/error checking
* TressFX/Epic Games Unreal 4.22 engine integration (patch under Epic Games Unreal GitHub repository)  (See https://github.com/GPUOpenSoftware/UnrealEngine/tree/TressFX-4.22)

*Hair Parameter Blending currently only available under TressFX/Unreal. Please see included documentation for more details.


### New in TressFX 4.0
* Hair is skinned directly, rather than through triangle stream-out.
* Signed distance field (SDF) collision, including compute shaders to generate the SDF from a dynamic mesh.
* New system for handling fast motion.
* Refactored to be more engine / API agnostic.
* Example code includes compute-based skinning and marching cubes generation.
* DirectX&reg; 12 support

### Integrating TressFX

With this release we aim to provide two things:

* A self-contained solution for hair simulation
* A reference renderer for hair

This is to provide an easy starting point for developers that want to integrate TressFX with their own technology. To that end, this update iterates on the "Engine Interface" that was introduced with TressFX 4.0. Both the renderer and the simulation go through this interface.
Developers that wish to implement their own hair rendering solution can use this interface to integrate into their own pipeline. They can use the renderer we provide as a guide. The hair simulation consists of asset loading code and the TressFX compute shaders which should be self contained enough to be integrated into another codebase without substantial changes to the source files.
To do this, a developer can implement all necessary functions using our Engine Interface, or replace it with their own calls and data structures (the latter method would be similar to the AMD TressFX/Epic Games Unreal integration, which is tightly bound to Unreal engine architecture).

For more information, please consult the included documentation (for both TressFX/Radeon&reg; Cauldron and TressFX/Epic Games Unreal engine integration).

### Prerequisites
* AMD Radeon&trade; GCN-based GPU (HD 7000 series or newer) or AMD Radeon&trade; RDNA-based GPU (5000 series or newer)
  * Or other DirectX&reg; 12 or Vulkan&reg; compatible GPU with Shader Model 6** support
* 64-bit Windows&reg; 7 (SP1 with the [Platform Update](https://msdn.microsoft.com/en-us/library/windows/desktop/jj863687.aspx)), Windows 8.1, or Windows 10
* Visual Studio 2019 or Visual Studio 2017
* Windows&reg; 10 required for DirectX&reg; 12
* CMake 3.4 (for Cauldron compatibility)
* Vulkan&reg; SDK 1.1.106 (for Cauldron compatibility)
* TressFX/Epic Games Unreal engine integration dependent upon Unreal 4.22 prerequisites (and will compile and run under DirectX&reg; 11 and DirectX&reg; 12)


** For the Cauldron framework to take advantage of DXIL support across DirectX&reg; 12 and Vulkan&reg;, Shader Model 6 was used. Shader Model 5 should work but was not explicitly tested.

### Getting started

#### Running the demo
* Install CMake 3.4 if you haven't already
* Install the Vulkan&reg; SDK for the Vulkan solution
* Visual Studio solutions for VS2019, VS2017, DirectX&reg; 12 or Vulkan&reg;, can be generated by running "GenerateSolutions.bat" in the "build folder.
* Compile the solution
* Run `TressFX_DX12.exe` or `TressFX_VK.exe` in `bin`.

#### Folder Structure
* src/ AMD Radeon&reg; Cauldron specific TressFX code shared between DirectX&reg; 12 and Vulkan&reg;
  * src/Math  TressFX math libraries 
  * src/Shaders  Shader sources in HLSL are here. Cauldron supports HLSL for both DirectX&reg; 12 and Vulkan&reg;.
  * src/TressFX  Platform agnostic TressFX files  
  * src/Common Cauldron specific, but rendering API agnostic, implementation
  * src/VK  Cauldron/Vulkan&reg; specific code (contains an engine interface implementation for Vulkan&reg;)
  * src/DX12  Cauldron/DirectX&reg; 12 specific code (contains an engine interface implementation for DirectX&reg; 12)
* maya-plugin/  Contains the python Autodesk&reg; Maya&reg; plugin. It is identical to the Exporter plugin used in Unreal
* doc/  Contains documentation including developer guide (shared by AMD Radeon&reg; Cauldron and Epic Games Unreal engine TressFX implementations)
* libs/cauldron/ Contains AMD Radeon&reg; Cauldron open source library for demo creation

### Previous releases
TressFX 4.1 is a substantial change from the prior TressFX 4.0 release.
A separate branch for TressFX 4.0 and 3.1.1 has been created for the convenience of users that have
been working with TressFX 3.
* [TressFX 4.0 branch](https://github.com/GPUOpen-Effects/TressFX/tree/4.0)
* [TressFX 3.1.1 branch](https://github.com/GPUOpen-Effects/TressFX/tree/3.1.1)

&nbsp;
&nbsp;
&nbsp;

**&copy; 2020 Advanced Micro Devices, Inc. All Rights Reserved.** 

### Attribution
* AMD, the AMD Arrow logo, AMD Radeon, and combinations thereof are trademarks of Advanced Micro Devices, Inc.
* Microsoft, Windows, DirectX, and combinations thereof are either trademarks or registered trademarks of Microsoft Corporation in the US and/or other countries. Vulkan and the Vulkan logo are registered trademarks of the Khronos Group Inc.
* Other product names used in this publication are for identification purposes only and may be trademarks of their respective companies.

**Disclaimer**

The information contained herein is for informational purposes only, and is subject to change without notice. While every precaution has been taken in the preparation of this document, it may contain technical inaccuracies, omissions and typographical errors, and AMD is under no obligation to update or otherwise correct this information.  Advanced Micro Devices, Inc. makes no representations or warranties with respect to the accuracy or completeness of the contents of this document, and assumes no liability of any kind, including the implied warranties of noninfringement, merchantability or fitness for particular purposes, with respect to the operation or use of AMD hardware, software or other products described herein.  No license, including implied or arising by estoppel, to any intellectual property rights is granted by this document.  Terms and limitations applicable to the purchase or use of AMD’s products are as set forth in a signed agreement between the parties or in AMD's Standard Terms and Conditions of Sale.
