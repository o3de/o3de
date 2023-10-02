{
    "Source" : "SilhouetteGather.azsl",
    "DepthStencilState" : {
        "Depth": 
        {
            "Enable": true, 
            "WriteMask" : "Zero",   // Avoid writing the depth
            "CompareFunc" : "Less"
        },
        "Stencil" :
        {
            "Enable" : true,
            "ReadMask" : "0x4",
            "WriteMask" : "0x0",
            "FrontFace" :
            {
                "Func" : "Equal",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Replace"
            },
            "BackFace" :
            {
                "Func" : "Equal",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Replace"
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
        "BlendAlphaSource" : "Zero",
        "BlendAlphaDest" : "One",
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
