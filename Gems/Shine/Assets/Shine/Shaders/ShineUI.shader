{
    "Source" : "ShineUI.azsl",

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

    "GlobalTargetBlendState" : {
        "Enable" : true, 
        "BlendSource" : "AlphaSource",
        "BlendDest" : "AlphaSourceInverse", 
        "BlendOp" : "Add"
    },

    "DrawList" : "Shinepass",

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
