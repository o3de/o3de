{
    "Source" : "ReflectionGlobalFullscreen.azsl",

    "RasterState" :
    {
        "CullMode" : "Back"
    },

    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : false
        },
        "Stencil" :
        {
            "Enable" : true,
            "ReadMask" : "0x7F",
            "WriteMask" : "0x00",
            "FrontFace" :
            {
                "Func" : "Equal",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Keep"
            }
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
            "PlusArguments": "--no-ms",
            "MinusArguments": ""
        }
    ]  
}
