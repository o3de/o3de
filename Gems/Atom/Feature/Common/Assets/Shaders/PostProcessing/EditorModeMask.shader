{
    "Source" : "EditorModeMask.azsl",
 
    "DepthStencilState" : { 
        "Depth" : { "Enable" : false }
    },

    "DrawList" : "editormodemask",

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