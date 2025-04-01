{
    "Source": "SplashScreenPass.azsl",

    "DepthStencilState" : { 
        "Depth" : { "Enable" : false }
    },

    "DrawList" : "forward",

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
