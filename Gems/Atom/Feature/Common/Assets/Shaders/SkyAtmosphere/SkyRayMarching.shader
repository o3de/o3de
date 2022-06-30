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
    "BlendState": {
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
    }
}
