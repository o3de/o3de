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

    "BlendState": {
        "Enable": true,
        "BlendSource": "AlphaSource",
        "BlendDest": "AlphaSourceInverse",
        "BlendOp": "Add"
    },

    "DrawList": "transparent"
}
