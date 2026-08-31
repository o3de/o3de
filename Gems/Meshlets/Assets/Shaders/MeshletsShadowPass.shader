{
    "Source" : "./MeshletsShadowPass.azsl",

    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : true,
            "CompareFunc" : "LessEqual"
        }
    },

    "DrawList" : "shadow",

    "RasterState" :
    {
        "depthBias" : "10",
        "depthBiasSlopeScale" : "4"
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MeshletsShadowPassVS",
          "type": "Vertex"
        }
      ]
    }
}
