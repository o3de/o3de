{ 
    "Source" : "SceneAndViewSrgs.azsl",

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
    
    "AddBuildArguments"
        : { "dxc" : ["-fspv-target-env=vulkan1.2"] },

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
            "RemoveBuildArguments": {
                "azslc": ["--strip-unused-srgs"]
            }
        }
    ]  
}
