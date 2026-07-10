{
    "Source": "ViewModeWireframe.azsl",
    "DepthStencilState": {
        "Depth": {
            "Enable": true,
            "WriteMask": "Zero",
            "CompareFunc": "GreaterEqual"
        }
    },
    "RasterState": {
        "FillMode": "Wireframe",
        "CullMode": "None",
        "DepthBias": 100,
        "DepthBiasSlopeScale": 2.0
    },
    "GlobalTargetBlendState": {
        "Enable": true,
        "BlendSource": "AlphaSource",
        "BlendDest": "AlphaSourceInverse",
        "BlendOp": "Add"
    },
    "DrawList": "viewmodewireframe",
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
    }
}
