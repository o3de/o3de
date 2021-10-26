{ 
    "Source" : "HairShortCutGeometryShading.azsl",
    "DrawList" : "HairGeometryShadingDrawList",    

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
        "BlendSource" : "One",
        "BlendDest" : "One",
        "BlendOp" : "Add",
        "BlendAlphaSource" : "One",
        "BlendAlphaDest" : "One",
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
          "name": "HairShortCutGeometryColorPS",
          "type": "Fragment"
        }
      ]
    }
}
