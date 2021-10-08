{
    "Source" : "Shadowmap",

    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "LessEqual" }
    },

    "DrawList" : "shadow",

    "RasterState" :
    {
        "depthBias" : "0",
        "depthBiasSlopeScale" : "0"        
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
