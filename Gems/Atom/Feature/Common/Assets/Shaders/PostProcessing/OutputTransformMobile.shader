{ 
    "Source" : "OutputTransform.azsl",
    "Definitions": ["MERGE_MANUAL_EXPOSURE=1"],

    "DepthStencilState" : {
        "Depth" : { "Enable" : false }
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
