{ 
    "Source" : "DeferredFog.azsl",

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

    "BlendState" : {
        "Enable" : true,
        "BlendSource" : "AlphaSource",
        "BlendDest" : "AlphaSourceInverse",
        "BlendOp" : "Add"
    },

    "DrawList" : "forward",

    "CompilerHints" : { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : false
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MainVS",
          "type": "Vertex"
        },
        {
          "name": "MainPS",
          "type": "Fragment"
        }
      ]
    }   
}
