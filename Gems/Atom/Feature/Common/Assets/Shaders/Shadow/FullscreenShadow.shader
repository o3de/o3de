{
    "Source" : "FullscreenShadow.azsl",

    "RasterState" :
    {
        "CullMode" : "None"
    },

    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : false
        }
    },

    "ProgramSettings":
    {
        "EntryPoints":
        [
            {
                "name": "MainCS",
                "type": "Compute"
            }
        ]
    }
}
