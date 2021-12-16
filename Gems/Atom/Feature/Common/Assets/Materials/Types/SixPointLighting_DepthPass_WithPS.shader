{
    "Source" : "./SixPointLighting_DepthPass_WithPS.azsl",

    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "GreaterEqual" }
    },

    "CompilerHints" : { 
        "DisableOptimizations" : false
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "DepthPassVS",
          "type": "Vertex"
        },
        {
          "name": "DepthPassPS",
          "type": "Fragment"
        }
      ]
    },

    "DrawList" : "depth"
}
