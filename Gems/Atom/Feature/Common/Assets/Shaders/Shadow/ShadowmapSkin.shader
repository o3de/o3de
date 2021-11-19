{
    "Source" : "ShadowmapSkin.azsl",

    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "LessEqual" }
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
          "name": "MainVS",
          "type": "Vertex"
        }
      ]
    }
}
