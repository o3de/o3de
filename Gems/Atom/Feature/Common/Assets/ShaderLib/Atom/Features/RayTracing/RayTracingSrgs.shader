{ 
    "Source" : "RayTracingSrgs.azsl",

    "AddBuildArguments": {
        "dxc": ["-fspv-target-env=vulkan1.2"]
    },

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : false 
        },
        "Stencil" :
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
            "Name": "",
            "AddBuildArguments": {
                "azslc": ["--no-alignment-validation"]
            },
            "RemoveBuildArguments": {
                "azslc": ["--strip-unused-srgs"]
            }
        }
    ]  
}
