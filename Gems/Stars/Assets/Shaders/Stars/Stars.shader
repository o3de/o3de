{
  "Source": "Stars.azsl",
  "DepthStencilState": {
    "Depth": {
      "Enable": true,
      "CompareFunc": "GreaterEqual"
    }
  },
  "BlendState" : {
      "Enable" : true,
      "BlendSource" : "AlphaSource",
      "BlendDest" : "One",
      "BlendOp" : "Add"
  },
  "DrawList": "forward",
  "ProgramSettings": {
    "EntryPoints": [
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
