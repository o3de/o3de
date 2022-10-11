{
    "Source" : "FullscreenShadow.azsl",

    "RasterState" :
    {
        "CullMode" : "None"
    },

    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : false
        }
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
    },

    "Supervariants":
    [
        {
            "Name": "NoMSAA",
                "AddBuildArguments" : {
                "azslc": ["--no-ms"]
            }
        }
    ]

    // Todo: test Compute Shader version with async compute and LDS optimizations
    // "ProgramSettings" :
    // {
    //     "EntryPoints":
    //     [
    //         {
    //             "name" : "MainCS",
    //             "type" : "Compute"
    //         }
    //     ]
    // }

}
