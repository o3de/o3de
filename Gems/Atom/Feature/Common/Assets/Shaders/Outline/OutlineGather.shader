{
    "Source" : "OutlineGather.azsl",
    "DepthStencilState" : {
        "Depth": 
        {
            "Enable": false,  //required to bind depth buffer SRV
            "CompareFunc" : "Always"
        }
    },
    "DrawList": "outline",
    "RasterState": { "CullMode": "None" },
    "ProgramSettings": {
        "EntryPoints": [
        {
            "name": "MainVS",
            "type" : "Vertex"
        },
        {
            "name": "MainPS",
            "type" : "Fragment"
        }
        ]
    }
}
