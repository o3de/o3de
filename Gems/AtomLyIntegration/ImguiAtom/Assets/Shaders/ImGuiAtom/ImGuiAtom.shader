{
 
    "Source" : "ImGuiAtom.azsl",

    
    "RasterState" : { "CullMode" : "None" },

    "DepthStencilState" : { 
        "Depth" : { "Enable" : false, "CompareFunc" : "GreaterEqual" }
    },

    "BlendState" : {
        "Enable" : true,
        "BlendSource" : "One",
        "BlendAlphaSource" : "One",
        "BlendDest" : "AlphaSourceInverse",
        "BlendAlphaDest" : "AlphaSourceInverse",
        "BlendAlphaOp" : "Add"
    },

    "DrawList" : "imgui",

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
