{ 
    "Source" : "HairRenderingResolvePPLL.azsl",

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : true,    // The resolve will write the closest hair depth
            // Pixels that don't belong to the hair will be discarded, otherwise if a fragment
            // already exists in the list, it passed the depth test in the fill pass
            "CompareFunc" : "Always"    
        },
        "Stencil" :
        {
            "Enable" : false
        }
    },

    // Note that in the original TressFX 4.1 a blend operation is used with source One and destination AlphaSource.
    // In our current implementation alpha blending is not required as the backbuffer is being sampled to create 
    // the proper background color. This will prevent having the slight silhuette that the original implementation 
    // sometime had.
    "BlendState" : 
    {
        "Enable" : false
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "FullScreenVS",
          "type": "Vertex"
        },
        {
          "name": "PPLLResolvePS",
          "type": "Fragment"
        }
      ]
    }
}
