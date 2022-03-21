{
    "Source" : "EditorModeMask.azsl",
 
    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "GreaterEqual" }
    },

    "DrawList" : "editormodemask",

    //"RasterState" :
    //{
    //    "depthBias" : "10",
    //    "depthBiasSlopeScale" : "4"
    //},

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