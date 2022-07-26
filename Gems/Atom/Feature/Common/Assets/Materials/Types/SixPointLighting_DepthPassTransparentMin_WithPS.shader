{
    "Source" : "./SixPointLighting_DepthPass_WithPS.azsl",

    "RasterState": { "CullMode": "None" },

    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "GreaterEqual" }
    },

    "CompilerHints" : { 
        "DisableOptimizations" : false
    },

    "ProgramSettings" : 
    {
        "EntryPoints":
        [
            {
                "name": "DepthPassVS",
                "type" : "Vertex"
            },
            {
                "name": "DepthPassPS",
                "type": "Fragment"
            }
        ] 
    },

    "DrawList" : "depthTransparentMin"
}
