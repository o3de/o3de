{
    "Source": "UnlitDepthPass.azsl",

    "DepthStencilState": {
        "Depth": {
            "Enable": true,
            "CompareFunc": "GreaterEqual"
        }
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

    "DrawList": "depth"
}
