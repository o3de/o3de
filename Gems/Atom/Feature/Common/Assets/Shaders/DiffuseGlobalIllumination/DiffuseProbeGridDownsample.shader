{
    "Source" : "DiffuseProbeGridDownsample.azsl",

    "RasterState" :
    {
        "CullMode" : "Back"
    },

    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : true,	// required to bind the depth buffer SRV
            "CompareFunc" : "Always"
        }
    },

    "DrawList" : "forward",

    "ProgramSettings":
    {
        "EntryPoints":
        [
            {
                "name": "MainVS",
                "type": "Vertex"
            },
            {
                "name": "MainPS",
                "type": "Fragment"
            }
        ]
    },

    "Supervariants":
    [
        {
            "Name": "NoMSAA",
            "PlusArguments": "--no-ms",
            "MinusArguments": ""
        }
    ]
}
