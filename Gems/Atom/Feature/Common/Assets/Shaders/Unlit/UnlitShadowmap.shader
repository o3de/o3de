{
    "Source": "UnlitDepthPass.azsl",

    "DepthStencilState": {
        "Depth": {
            "Enable": true,
            "CompareFunc": "LessEqual"
        }
    },

    "RasterState": {
        "depthBias": "10",
        "depthBiasSlopeScale": "4"
    },

    "ProgramSettings": {
        "EntryPoints": [
            {
                "name": "DepthVS",
                "type": "Vertex"
            },
            {
                "name": "DepthPS",
                "type": "Fragment"
            }
        ]
    },

    "DrawList": "shadow"
}
