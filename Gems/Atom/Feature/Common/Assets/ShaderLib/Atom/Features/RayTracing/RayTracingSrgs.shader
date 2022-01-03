{ 
    "Source" : "RayTracingSrgs.azsl",

    "CompilerHints":
    {
        "DxcAdditionalFreeArguments" : "-fspv-target-env=vulkan1.2"
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
            "PlusArguments": "--no-alignment-validation",
            "MinusArguments": "--strip-unused-srgs"
        }
    ]  
}
