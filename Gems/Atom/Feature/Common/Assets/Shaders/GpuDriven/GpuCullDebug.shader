{
    "Source": "GpuCullDebug.azsl",
    "DepthStencilState": {
        "Depth": {
            "Enable": true,
            "WriteMask": "Zero",
            "CompareFunc": "GreaterEqual"
        }
    },
    "RasterState": {
        "CullMode": "Back"
    },
    "ProgramSettings": {
        "EntryPoints": [
            {
                "name": "GpuCullDebugVS",
                "type": "Vertex"
            },
            {
                "name": "GpuCullDebugPS",
                "type": "Fragment"
            }
        ]
    }
}
