{
    "Source" : "SkyViewLUT.azsl",

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
                "name": "SkyViewLutPS",
                "type" : "Fragment"
            }
        ]
    }
}
