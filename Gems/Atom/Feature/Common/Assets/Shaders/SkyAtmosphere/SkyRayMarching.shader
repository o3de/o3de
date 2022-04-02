{
    "Source" : "SkyRayMarching.azsl",
    "DepthStencilState" : {
        "Depth": 
        {
            "Enable": false,  //required to bind depth buffer SRV
            "CompareFunc" : "Always"
            //"CompareFunc" : "GreaterEqual"
        }
    },
    "DrawList": "forward",

    "BlendState": {
        "Enable": true,
        "BlendSource": "One",
        //"BlendSource": "AlphaSource",
        "BlendDest": "One",
        "BlendOp": "Add"
        //"BlendAlphaSource": "Zero",
        //"BlendAlphaDest": "One",
        //"BlendAlphaOp": "Add"
    },
    "ProgramSettings": {
        "EntryPoints": [
        {
            "name": "MainVS",
            "type" : "Vertex"
        },
        {
            "name": "RayMarchingPS",
            "type" : "Fragment"
        }
        ]
    },
    "CompilerHints" : 
    {
        "DxcDisableOptimizations": true,
        "DxcGenerateDebugInfo" : true
    }
}
