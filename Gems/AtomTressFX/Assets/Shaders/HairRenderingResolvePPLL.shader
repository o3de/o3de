{ 
    "Source" : "HairRenderingResolvePPLL.azsl",
	
    "CompilerHints" : 
    { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : false
    },

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : true,    // The resolve will write the closest hair depth
            // Pixels that don't belong to the hair will be discarded, otherwise if a fragment
            // exist in the list, it already passed the depth test
            "CompareFunc" : "Always"    
        },
        "Stencil" :
        {
            "Enable" : false
        }
    },

    "BlendState" : 
    {
        "Enable" : false,
//        "Enable" : true,
        "BlendSource" : "AlphaSource",
        "BlendDest" : "AlphaSourceInverse",
        "BlendOp" : "Add",
//        "BlendSource" : "One",
//        "BlendDest" : "AlphaSource",
//        "BlendOp" : "Add",

        "BlendAlphaSource" : "Zero",
        "BlendAlphaDest" : "Zero",
        "BlendAlphaOp" : "Add"
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
