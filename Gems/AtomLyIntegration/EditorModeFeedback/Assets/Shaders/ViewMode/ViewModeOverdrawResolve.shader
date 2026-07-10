{
    "Source": "ViewModeOverdrawResolve.azsl",
    "DepthStencilState": {
        "Depth": {
            "Enable": false
        }
    },
    "GlobalTargetBlendState": {
        "Enable": true,
        "BlendSource": "AlphaSource",
        "BlendDest": "AlphaSourceInverse",
        "BlendOp": "Add"
    },
    "DrawList": "viewmodeoverdrawresolve",
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
