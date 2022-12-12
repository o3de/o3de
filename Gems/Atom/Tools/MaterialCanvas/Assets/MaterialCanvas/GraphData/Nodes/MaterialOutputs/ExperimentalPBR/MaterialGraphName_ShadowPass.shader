{
    "Source": "./MaterialGraphName_ShadowPass.azsl",
    "DrawList": "shadow",
    "DepthStencilState": {
        "depth": {
            "compareFunc": "LessEqual"
        }
    },
    "RasterState": {
        "depthBias": 10,
        "depthBiasSlopeScale": 4.0
    },
    "ProgramSettings": {
        "EntryPoints": [
            {
                "Name": "MainVS"
            }
        ]
    }
}