{
    "Source": "./ShadowCatcher.azsl",

    "DepthStencilState": {
        "Depth": {
            "Enable": true,
            "CompareFunc": "GreaterEqual",
            "WriteMask": "Zero"
        }
    },

    "RasterState": { "CullMode": "None" },

    "GlobalTargetBlendState": {
        "Enable": true,
        "BlendSource": "AlphaSource",
        "BlendDest": "AlphaSourceInverse",
        "BlendOp": "Add"
    },

    "ProgramSettings" : 
    {
        "EntryPoints":
        [
            {
                "name": "ShadowCatcherVS",
                "type" : "Vertex"
            },
            {
                "name": "ShadowCatcherPS",
                "type" : "Fragment"
            }
        ] 
    },

    "DrawList": "transparent"
}
