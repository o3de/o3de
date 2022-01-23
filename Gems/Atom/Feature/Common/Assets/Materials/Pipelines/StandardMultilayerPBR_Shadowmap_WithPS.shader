{
    "Source" : "./StandardMultilayerPBR_Shadowmap_WithPS.azsl",

    "Requirements": [
      "VertexLocalToWorld",
      "EvaluateUVs",
      "EvaluateWorldSpaceTBN",
      "EvaluateMultilayerPixelDepth"
    ],

    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "LessEqual" }
    },

    "DrawList" : "shadow",

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
