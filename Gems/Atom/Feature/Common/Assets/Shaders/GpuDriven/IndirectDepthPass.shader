{
    "Source": "IndirectDepthPass.azsl",

    "DepthStencilState": {
        "Depth": { "Enable": true, "CompareFunc": "GreaterEqual" }
    },

    "ProgramSettings":
    {
        "EntryPoints":
        [
            {
                "name": "IndirectDepthVS",
                "type": "Vertex"
            }
        ]
    }
}
