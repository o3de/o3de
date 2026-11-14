{
    "Source": "WhiteBoxWireframe.azsl",

    "DepthStencilState": {
        "Depth": {
            "Enable": true,
            "CompareFunc": "GreaterEqual",
            "writeMask": "Zero"
        }
    },

    "RasterState": {
        "depthBias": "200",
        "depthBiasSlopeScale": "1.0"
    },

    "GlobalTargetBlendState": {
        "Enable": true,
        "BlendSource": "One",
        "BlendDest": "AlphaSourceInverse",
        "BlendOp": "Add"
    },

    "ProgramSettings": {
        "EntryPoints": [
            {
                "name": "MainVS",
                "type": "Vertex"
            },
            {
                "name": "MainGS",
                "type": "Geometry"
            },
            {
                "name": "MainPS",
                "type": "Fragment"
            }
        ]
    },

    "DrawList": "transparent"
} 
