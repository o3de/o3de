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
            "Enable" : true,
            "CompareFunc" : "Always"  // Verified to be the correct direction although LessEqual in TressFX
        },
        "Stencil" :
        {
            "Enable" : false
        }
    },

    "BlendState" : 
    {
        "Enable" : true,
        "BlendSource" : "One",
        "BlendDest" : "AlphaSource",
        "BlendOp" : "Add",

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
