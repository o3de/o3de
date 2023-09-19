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
        "BlendSource" : "AlphaSource",
        "BlendDest" : "AlphaSourceInverse"
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
