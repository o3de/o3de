{ 
    "Source" : "ReflectionProbeRenderInner.azsl",

    "RasterState" :
    {
        "CullMode" : "Front"
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
            "ReadMask" : "0x3F",
            "WriteMask" : "0x00",
            "BackFace" :
            {
                "Func" : "LessEqual",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Keep"
            }
        }
    },

    "DrawList" : "reflectionproberenderinner",

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
