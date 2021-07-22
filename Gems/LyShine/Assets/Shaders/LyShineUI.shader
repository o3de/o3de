{
    "Source" : "LyShineUI",

    "DepthStencilState" : { 
        "Depth" : { 
            "Enable" : false, 
            "CompareFunc" : "Always"
        }
    },

    "RasterState" : {
        "DepthClipEnable" : false,
        "CullMode" : "None"
    },

    "BlendState" : {
        "Enable" : true, 
        "BlendSource" : "AlphaSource",
        "BlendDest" : "AlphaSourceInverse", 
        "BlendOp" : "Add"
    },

    "DrawList" : "uicanvas",

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
    }
}
