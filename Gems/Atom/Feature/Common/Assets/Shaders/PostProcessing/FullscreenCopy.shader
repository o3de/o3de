{ 
    "Source" : "FullscreenCopy.azsl",

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
    },
    "Supervariants":
    [
        {
            "Name": "",
            "AddBuildArguments": {
                "azslc": ["--no-subpass-input"]
            }
        },
        {
            "Name": "SubpassInput",
            "RemoveBuildArguments": {
                "azslc": ["--no-subpass-input"]
            }
        }
    ]
}
