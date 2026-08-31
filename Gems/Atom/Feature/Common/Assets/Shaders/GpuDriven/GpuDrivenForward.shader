{
    "Source": "GpuDrivenForward.azsl",

    "DepthStencilState": {
        "Depth": { "Enable": true, "CompareFunc": "GreaterEqual" }
    },

    "ProgramSettings":
    {
        "EntryPoints":
        [
            {
                "name": "GpuDrivenForwardVS",
                "type": "Vertex"
            },
            {
                "name": "GpuDrivenForwardPS",
                "type": "Fragment"
            }
        ]
    }
}
