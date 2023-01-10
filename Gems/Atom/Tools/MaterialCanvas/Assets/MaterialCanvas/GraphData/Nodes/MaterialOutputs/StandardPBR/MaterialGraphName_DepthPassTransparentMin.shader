{
    "Source" : "./MaterialGraphName_DepthPass.azsl",

    "RasterState": { "CullMode": "None" },

    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "GreaterEqual" }
    },

    "ProgramSettings" : 
    {
        "EntryPoints":
        [
            {
                "name": "MainVS",
                "type" : "Vertex"
            }
        ] 
    },

    "DrawList" : "depthTransparentMin"
}
