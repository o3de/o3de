{ 
    "Source" : "HairShortCutResolveColor.azsl",

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : false   // Avoid comparing depth
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
          "name": "HairShortCutResolveColorPS",
          "type": "Fragment"
        }
      ]
    }
}
