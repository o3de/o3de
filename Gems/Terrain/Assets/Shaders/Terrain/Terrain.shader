{
 
    "Source" : "Terrain",

    
    "RasterState" : { "CullMode" : "None" },

    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "GreaterEqual" }
    },

    "BlendState" : {
        "Enable" : true,
        "BlendSource" : "One",
        "BlendAlphaSource" : "One",
        "BlendDest" : "AlphaSourceInverse",
        "BlendAlphaDest" : "AlphaSourceInverse",
        "BlendAlphaOp" : "Add"
    },

    "DrawList" : "forward",

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
