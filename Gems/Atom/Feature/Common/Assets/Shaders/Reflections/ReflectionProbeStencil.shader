{ 
    "Source" : "ReflectionProbeStencil.azsl",

    "RasterState" :
    {
        "CullMode" : "None"
    },

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : true,
            "WriteMask" : "Zero",
            "CompareFunc" : "GreaterEqual"
        },
        "Stencil" : 
        { 
            "Enable" : true,
            "ReadMask" : "0x3F",
            "WriteMask" : "0x3F",
            "FrontFace" :
            {
                "Func" : "Less",
                "DepthFailOp" : "DecrementSaturate",
                "FailOp" : "Keep",
                "PassOp" : "Keep"
            },
            "BackFace" :
            {
                "Func" : "Less",
                "DepthFailOp" : "IncrementSaturate",
                "FailOp" : "Keep",
                "PassOp" : "Keep"
            }
        }
    },

    "DrawList" : "reflectionprobestencil",

    "ProgramSettings":
    {
        "EntryPoints":
        [
            {
                "name": "MainVS",
                "type": "Vertex"
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
