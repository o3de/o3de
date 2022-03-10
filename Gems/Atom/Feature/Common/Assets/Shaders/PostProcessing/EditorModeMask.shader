{
    "Source" : "EditorModeMask.azsl",
 
    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "Always" }
    },

    "DrawList" : "editormodemask",

    //"RasterState" :
    //{
    //    "depthBias" : "10",
    //    "depthBiasSlopeScale" : "4"
    //},

    "BlendState" : {
        "Enable" : true,
        "BlendSource" : "AlphaSource",
        "BlendDest" : "AlphaSourceInverse",
        "BlendOp" : "Maximum"
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