{
    "Source" : "SkyTransmittanceLUT.azsl",

    "DepthStencilState" : {
        "Depth" : { "Enable" : false }
    },

    "ProgramSettings": 
    {
        "EntryPoints": [
            {
                "name": "MainVS",
                "type" : "Vertex"
            },
            {
                "name": "SkyTransmittanceLUTPS",
                "type" : "Fragment"
            }
        ]
    }
}
