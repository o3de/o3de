{ 
    "Source" : "LuminanceHeatmap.azsl",

    "DepthStencilState" : {
        "Depth" : { "Enable" : false }
    },
    
    "BlendState" : {
        "Enable" : true,
        "BlendSource" : "AlphaSource",
        "BlendDest" : "AlphaSourceInverse",
        "BlendOp" : "Add"
    },

    "DrawList" : "forward",
    
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
