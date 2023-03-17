{
    "Source" : "./MaterialGraphName_Depth.azsl",

    "Definitions" : ["SHADOWMAP=1"],

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
        },
        {
          "name": "MainPS",
          "type": "Fragment"
        }
      ]
    }
}
