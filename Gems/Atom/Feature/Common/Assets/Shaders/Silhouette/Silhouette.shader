{
    "Source" : "Silhouette.azsl",
    "DepthStencilState" : {
        "Depth": 
        {
            "Enable": false,
            "CompareFunc" : "Always"
        }
    },
    "GlobalTargetBlendState": {
        "Enable": true,
        "BlendSource" : "One",
        "BlendAlphaSource" : "One",
        "BlendDest" : "AlphaSourceInverse",
        "BlendAlphaDest" : "AlphaSourceInverse",
        "BlendAlphaOp" : "Add"
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
