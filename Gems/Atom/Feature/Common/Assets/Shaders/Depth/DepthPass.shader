{
    "Source" : "./DepthPass.azsl",

    "Requirements": [
        "VertexLocalToWorld"
    ],

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

    "DrawList" : "depth"
}
