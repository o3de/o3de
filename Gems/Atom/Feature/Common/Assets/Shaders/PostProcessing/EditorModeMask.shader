{
    "Source" : "EditorModeMask.azsl",
 
    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "Always" }
    },

    "DrawList" : "editormodemask",

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