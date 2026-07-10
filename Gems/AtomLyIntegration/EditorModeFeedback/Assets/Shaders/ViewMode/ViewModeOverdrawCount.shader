{
    "Source": "ViewModeOverdrawCount.azsl",
    "DepthStencilState": {
        "Depth": {
            "Enable": true,
            "WriteMask": "Zero",
            "CompareFunc": "GreaterEqual"
        }
    },
    "RasterState": {
        "CullMode": "Back",
        "DepthBias": 100,
        "DepthBiasSlopeScale": 2.0
    },
    "DrawList": "viewmodeoverdraw",
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
