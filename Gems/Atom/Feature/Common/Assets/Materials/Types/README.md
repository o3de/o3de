# Upcoming material system changes

Currently, `.materialtype` files specify a set of shaders for each pass in the rendering pipeline.
For example, the `StandardPBR.materialtype` asset specifies the following shaders:

```json
[
    {
        "file": "./StandardPBR_ForwardPass.shader",
        "tag": "ForwardPass"
    },
    {
        "file": "./StandardPBR_ForwardPass_EDS.shader",
        "tag": "ForwardPass_EDS"
    },
    {
        "file": "./StandardPBR_LowEndForward.shader",
        "tag": "LowEndForward"
    },
    {
        "file": "./StandardPBR_LowEndForward_EDS.shader",
        "tag": "LowEndForward_EDS"
    },
    {
        "file": "./StandardPBR_MultiViewForward.shader",
        "tag": "MultiViewForward"
    },
    {
        "file": "./StandardPBR_MultiViewForward_EDS.shader",
        "tag": "MultiViewForward_EDS"
    },
    {
        "file": "./StandardPBR_MultiViewTransparentForward_EDS.shader",
        "tag": "MultiViewTransparentForward"
    },
    {
        "file": "Shaders/Shadow/Shadowmap.shader",
        "tag": "Shadowmap"
    },
    {
        "file": "./StandardPBR_Shadowmap_WithPS.shader",
        "tag": "Shadowmap_WithPS"
    },
    {
        "file": "Shaders/Depth/DepthPass.shader",
        "tag": "DepthPass"
    },
    {
        "file": "./StandardPBR_DepthPass_WithPS.shader",
        "tag": "DepthPass_WithPS"
    },
    {
        "file": "Shaders/MotionVector/MeshMotionVector.shader",
        "tag": "MeshMotionVector"
    },
    {
        "file": "Shaders/Depth/DepthPassTransparentMin.shader",
        "tag": "DepthPassTransparentMin"
    },
    {
        "file": "Shaders/Depth/DepthPassTransparentMax.shader",
        "tag": "DepthPassTransparentMax"
    }        
]
```

**This will be changing in a future release** to a material type description that specifies shader snippets (aka material functions)
instead of explicit shaders.

## Why is it changing?

There are two primary reasons to move to a different scheme.

1. Material types are strongly coupled to the rendering pipeline. If a user wants to change the pipeline, or the engine wants to use, for example, a custom pipeline for mobile, or VR, this isn't possible today without cloning existing material types and changing the shader array.
2. The material canvas work that has been prioritized to allow artist-driven material customization benefits from a more modular construction of materials. For example, we'd like to apply a "wind graph" and mix and match that with a "foliage graph" to describe the appearance of some foliage. The current material type description couples all the geometric passes with the material and lighting passes, which makes this sort of decomposition difficult.

## What is it changing to?

The best way to understand how this is changing is to inspect the current structure of `EnhancedPBR_ForwardPass.azsl` and `StandardPBR_ForwardPass.azsl`.
These shaders start with a number of includes to specify the SRG as follows:

```hlsl
#include "StandardPBR_Common.azsli"
#include <Atom/Features/PBR/DefaultObjectSrg.azsli>
```

Later, it includes a number of material functions, for example:

```hlsl
#include "MaterialFunctions/EvaluateStandardSurface.azsli"
#include "MaterialFunctions/EvaluateTangentFrame.azsli"
#include "MaterialFunctions/ParallaxDepth.azsli"
#include "MaterialFunctions/StandardGetNormalToWorld.azsli"
#include "MaterialFunctions/StandardGetObjectToWorld.azsli"
#include "MaterialFunctions/StandardGetAlphaAndClip.azsli"
#include "MaterialFunctions/StandardTransformUvs.azsli"
```

The material function headers define functions that may later be overridden using material graphs.

Finally, in the case of the standard surface shader, it includes an implementation file:

```hlsl
#include "StandardSurface_ForwardPass.azsli"
```

This file, if you inspect it, _makes no reference to `MaterialSrg`_, and furthermore, does not include files needed to implement any of the material functions.
In other words, the structure of the standard pbr forward shader is such that it can be assembled with different components, specifing the SRG, material functions, and implementation.

In the future, a material pipeline abstraction will allow the `materialtype` asset to specify _only_ the material function files, and the tuple of `materialtype` and `materialpipeline` will allow the material builder to assemble the shader on behalf of the user. The final piece to the puzzle is that (again, in the future), material canvas (in active development) can produce material functions to replace the built-in ones.
