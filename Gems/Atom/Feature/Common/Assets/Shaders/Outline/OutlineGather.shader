{
    "Source" : "OutlineGather.azsl",
    "DepthStencilState" : {
        "Depth": 
        {
            "Enable": true,  //required to bind depth buffer SRV
            "CompareFunc" : "LessEqual"
        }
    },
    "DrawList": "outline",
    "RasterState": { 
        "CullMode": "Back",
        "depthBias" : "0",
        "depthBiasSlopeScale" : "0"        
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
