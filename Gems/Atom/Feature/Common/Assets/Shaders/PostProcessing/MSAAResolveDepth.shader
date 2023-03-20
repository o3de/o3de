{ 
    "Source" : "MSAAResolveDepth.azsl",

    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "Always" }
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
