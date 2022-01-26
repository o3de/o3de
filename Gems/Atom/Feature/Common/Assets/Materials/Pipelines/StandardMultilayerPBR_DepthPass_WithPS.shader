{
    "Source" : "./StandardMultilayerPBR_DepthPass_WithPS.azsl",
    
    "Requirements": [
      "VertexLocalToWorld",
      "EvaluateUVs",
      "EvaluateWorldSpaceTBN",
      "EvaluateMultilayerPixelDepth"
    ],

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
          "name": "MainVS",
          "type": "Vertex"
        },
        {
          "name": "MainPS",
          "type": "Fragment"
        }
      ]
    },

    "DrawList" : "depth"
}
