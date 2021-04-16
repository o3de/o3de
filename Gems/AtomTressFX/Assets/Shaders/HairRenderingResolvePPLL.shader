{ 
    "Source" : "HairRenderingResolvePPLL.azsl",
	
    "CompilerHints" : 
    { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : true
    },

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : false 
        },
        "Stencil" :
        {
            "Enable" : false
        }
    },

    "BlendState" : 
    {
        "Enable" : true,

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
