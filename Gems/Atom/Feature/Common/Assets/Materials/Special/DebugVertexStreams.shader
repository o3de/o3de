{
    "Source" : "DebugVertexStreams.azsl",

    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "GreaterEqual" }
    },

    "DrawList" : "auxgeom",

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
