{
    "Source" : "./EnhancedPBR_Shadowmap_WithPS.azsl",

    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "LessEqual" }
    },

    "DrawList" : "shadow",

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
