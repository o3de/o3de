{
    "Source" : "ReflectionScreenSpaceComposite.azsl",

    "RasterState" :
    {
        "CullMode" : "Back"
    },

    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : false
        }
    },

    "GlobalTargetBlendState":
    {
        "Enable": true,
        "BlendSource": "AlphaSource",
        "BlendDest": "AlphaSourceInverse",
        "BlendOp": "Add"
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
    }, 

    "Supervariants":
    [
        {
            "Name": "NoMSAA",
            "AddBuildArguments": {
                "azslc": ["--no-ms"]
            }
        }
    ]  
}
