{
    "Source": "UnlitTransparent.azsl",

    "DepthStencilState": {
        "Depth": {
            "Enable": true,
            "CompareFunc": "GreaterEqual",
            "writeMask": "Zero"
        }
    },

    "GlobalTargetBlendState": {
        "Enable": true,
        "BlendSource": "One",
        "BlendAlphaSource": "One",
        "BlendDest": "AlphaSourceInverse",
        "BlendAlphaDest": "AlphaSourceInverse",
        "BlendOp": "Add",
        "BlendAlphaOp": "Add"
    },

    "ProgramSettings": {
        "EntryPoints": [
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

    "DrawList": "transparent"
}