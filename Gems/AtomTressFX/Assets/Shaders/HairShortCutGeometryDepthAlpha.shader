{ 
    "Source" : "HairShortCutGeometryDepthAlpha.azsl",
    "DrawList" : "HairGeometryDepthAlphaDrawList",

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : true,
            "WriteMask" : "Zero",   // Avoid writing the depth
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
        "Enable" : true,
        "BlendSource" : "Zero",
        "BlendDest" : "ColorSource",
        "BlendOp" : "Add",
        "BlendAlphaSource" : "Zero",
        "BlendAlphaDest" : "AlphaSource",
        "BlendAlphaOp" : "Add"
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "RenderHairVS",
          "type": "Vertex"
        },
        {
          "name": "HairShortCutDepthsAlphaPS",
          "type": "Fragment"
        }
      ]
    },
    "DisabledRHIBackends": ["metal"]
}
