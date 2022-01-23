{
    "Source" : "./DepthPass.azsl",

    "Requirements": [
        "VertexLocalToWorld"
    ],

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
            }
        ] 
    },

    "DrawList" : "depthTransparentMin"
}
