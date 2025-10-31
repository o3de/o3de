{ 
    "Source" : "LightCullingHeatmap.azsl",

    "DepthStencilState" : {
        "Depth" : { "Enable" : false }
    },
    
    "GlobalTargetBlendState" : {
        "Enable" : true,
        "BlendSource" : "AlphaSource",
        "BlendDest" : "AlphaSourceInverse",
        "BlendOp" : "Add"
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
