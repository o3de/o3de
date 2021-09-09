{
    "Source" : "./Terrain_DepthPass.azsl",

    "DepthStencilState" :
    { 
        "Depth" :
        {
            "Enable" : true,
            "CompareFunc" : "GreaterEqual"
        }
    },

    "DrawList" : "depth",

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MainVS",
          "type": "Vertex"
        }
      ]
    }

}
