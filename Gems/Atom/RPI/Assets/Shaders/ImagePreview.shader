{
    "Source" : "ImagePreview.azsl",

    "DepthStencilState" : { 
        "Depth" : { "Enable" : false, "CompareFunc" : "GreaterEqual" }
    },

    "RasterState" : { "CullMode" : "None" },
        
    "DrawList": "forward",

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
