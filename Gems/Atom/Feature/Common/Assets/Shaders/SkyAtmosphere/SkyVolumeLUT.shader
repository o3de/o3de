{
    "Source" : "SkyVolumeLUT.azsl",

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
                "name": "SkyVolumeLUTCS",
                "type" : "Compute"
            }
        ]
    }
}
