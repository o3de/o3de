{
    "Source" : "SkyRayMarching.azsl",
    "DepthStencilState" : {
        "Depth": 
        {
            "Enable": false,  //required to bind depth buffer SRV
            "CompareFunc" : "Always"
        }
    },
    "DrawList": "forward",
    "GlobalTargetBlendState": {
        "Enable": true,
        "BlendSource": "One",
        "BlendDest": "One",
        "BlendOp": "Add"
    },
    "ProgramSettings": {
        "EntryPoints": [
        {
            "name": "MainVS",
            "type" : "Vertex"
        },
        {
            "name": "RayMarchingPS",
            "type" : "Fragment"
        }
        ]
    },
    "Supervariants":
    [
        {
            "Name": "NoMSAA",
            "AddBuildArguments": {
                "azslc": ["--no-ms"]
            }
        }
    ]
}
