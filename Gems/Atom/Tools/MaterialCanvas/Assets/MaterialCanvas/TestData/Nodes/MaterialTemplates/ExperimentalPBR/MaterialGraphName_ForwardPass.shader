{
    "Source": "./MaterialGraphName_ForwardPass.azsl",
    "DrawList": "forward",
    "DepthStencilState": {
        "depth": {
            "compareFunc": "GreaterEqual"
        },
        "stencil": {
            "enable": 1,
            "readMask": 0,
            "frontFace": {
                "passOp": "Replace"
            }
        }
    },
    "ProgramSettings": {
        "EntryPoints": [
            {
                "Name": "MainVS"
            },
            {
                "Name": "MainPS",
                "Type": "Fragment"
            }
        ]
    }
}