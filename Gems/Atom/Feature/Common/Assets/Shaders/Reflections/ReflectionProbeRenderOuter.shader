{ 
    "Source" : "ReflectionProbeRenderOuter.azsl",

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
            "ReadMask" : "0x7F",
            "WriteMask" : "0x00",
            "BackFace" :
            {
                "Func" : "Equal",
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

    "DrawList" : "reflectionproberenderouter",

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
