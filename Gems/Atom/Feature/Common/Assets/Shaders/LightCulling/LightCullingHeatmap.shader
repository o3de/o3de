{ 
    "Source" : "LightCullingHeatmap.azsl",

    "DepthStencilState" : {
        "Depth" : { "Enable" : false }
    },
    
    "BlendState" : {
        "Enable" : true,
        "BlendSource" : "AlphaSource",
        "BlendDest" : "AlphaSourceInverse",
        "BlendOp" : "Add"
    },

    "CompilerHints":
    {
        "DisableOptimizations":false
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
