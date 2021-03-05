{ 
    "Source" : "ReflectionProbeRenderOuter",

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
            "ReadMask" : "0xFF",
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
    }
}
