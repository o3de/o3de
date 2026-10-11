{
    "Source": "SceneColorDistortionTest.azsl",
    "DrawList": "sceneColor",
    "RasterState": { "CullMode": "None" },
    "DepthStencilState": {
        "Depth": { "Enable": true, "WriteMask": "Zero", "CompareFunc": "GreaterEqual" },
        "Stencil": { "Enable": false }
    },
    "ProgramSettings": {
        "EntryPoints": [
            { "name": "MainVS", "type": "Vertex" },
            { "name": "MainPS", "type": "Fragment" }
        ]
    }
}
