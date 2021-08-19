{ 
    "Source" : "HairRenderingFillPPLL.azsl",	
    "DrawList" : "hairFillPass",

    "CompilerHints" : 
    { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : true
    },

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : true,
             "CompareFunc" : "GreaterEqual"
            // Adi - in TressFX this is LessEqual
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
//        "BlendSource" : "AlphaSource",
//        "BlendDest" : "AlphaSourceInverse",
//        "BlendOp" : "Add"
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
