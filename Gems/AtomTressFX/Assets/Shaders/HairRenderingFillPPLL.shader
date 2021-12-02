{ 
    "Source" : "HairRenderingFillPPLL.azsl",	
    "DrawList" : "hairFillPass",

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : true,
             "CompareFunc" : "GreaterEqual"
            // Originally in TressFX this is LessEqual - Atom is using reverse sort
        },
        "Stencil" :
        {
            "Enable" : false
        }
    },

    "RasterState" :
    {
        "CullMode" : "None"
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
          "name": "RenderHairVS",
          "type": "Vertex"
        },
        {
          "name": "PPLLFillPS",
          "type": "Fragment"
        }
      ]
    }
}
