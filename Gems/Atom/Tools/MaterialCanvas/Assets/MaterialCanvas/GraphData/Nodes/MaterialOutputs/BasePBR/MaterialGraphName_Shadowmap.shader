{
    "Source": "./MaterialGraphName_Depth.azsl",

    "Definitions" : ["SHADOWMAP=1"] ,

    
    "DepthStencilState" : {
        "Depth" : { "Enable" : true, "CompareFunc" : "LessEqual" }
    },

    "DrawList" : "shadow",
    
    // Note that lights now expose their own bias controls.
    // It may be worth increasing their default values in the future and reducing the depthBias values encoded here.
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
