{
    "Source" : "SilhouetteGather.azsl",
    "DepthStencilState" : {
        "Depth": 
        {
            "Enable": false, 
            "WriteMask" : "Zero",   // Avoid writing the depth
            "CompareFunc" : "LessEqual"
        },
        "Stencil" :
        {
            "Enable" : true,
            "ReadMask" : "0x4",
            "WriteMask" : "0x0",
            "FrontFace" :
            {
                "Func" : "NotEqual",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Keep"
            },
            "BackFace" :
            {
                "Func" : "NotEqual",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Keep"
            }
        }
    },
    "DrawList": "silhouette",
    "RasterState": { 
        "CullMode": "Front"
    },

    "GlobalTargetBlendState": {
        "Enable": true,
        "BlendSource" : "One",
        "BlendDest" : "Zero",
        "BlendOp" : "Maximum",
        "BlendAlphaSource" : "One",
        "BlendAlphaDest" : "Zero",
        "BlendAlphaOp" : "Maximum"
    },
    "ProgramSettings": {
        "EntryPoints": [
        {
            "name": "MainVS",
            "type" : "Vertex"
        },
        {
            "name": "MainPS",
            "type" : "Fragment"
        }
        ]
    }
}
