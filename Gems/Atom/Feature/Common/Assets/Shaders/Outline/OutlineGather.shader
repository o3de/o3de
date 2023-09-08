{
    "Source" : "OutlineGather.azsl",
    "DepthStencilState" : {
        "Depth": 
        {
            "Enable": false,  //required to bind depth buffer SRV
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
    "DrawList": "outline",
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
