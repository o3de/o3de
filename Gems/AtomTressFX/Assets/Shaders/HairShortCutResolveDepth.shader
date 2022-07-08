{ 
    "Source" : "HairShortCutResolveDepth.azsl",

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : true,    // test the written depth and accept/discard based on the depth buffer
            "CompareFunc" : "GreaterEqual"
            // Originally in TressFX this is LessEqual - Atom is using reverse sort 
        },
        "Stencil" :
        {
            "Enable" : false
        }
    },

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
          "name": "HairShortCutResolveDepthPS",
          "type": "Fragment"
        }
      ]
    }
}
