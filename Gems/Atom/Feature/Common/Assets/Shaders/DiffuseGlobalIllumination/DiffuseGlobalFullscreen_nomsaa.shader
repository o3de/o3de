{
    "Source" : "DiffuseGlobalFullscreen_nomsaa",

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
            "ReadMask" : "0xFF",
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
        "BlendOp" : "Add",
        "BlendAlphaSource" : "Zero",
        "BlendAlphaDest" : "One",
        "BlendAlphaOp" : "Add"
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
    }
}
