{
    "Source" : "ReflectionComposite.azsl",

    "RasterState" :
    {
        "CullMode" : "Back"
    },

    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : false,
            "WriteMask" : "Zero",
            "CompareFunc" : "GreaterEqual"
        },
        "Stencil" :
        {
            "Enable" : true,
            "ReadMask" : "0x7F",
            "WriteMask" : "0x00",
            "FrontFace" :
            {
                "Func" : "LessEqual",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Keep"
            }
        }
    },

    "BlendState" : {
        "Enable" : true,
        "BlendSource" : "One",
        "BlendDest" : "One",
        "BlendOp" : "Add"
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
