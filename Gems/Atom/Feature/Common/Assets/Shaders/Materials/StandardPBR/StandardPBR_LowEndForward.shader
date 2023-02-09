{
    // Note: "LowEnd" shaders are for supporting the low end pipeline
    // These shaders can be safely added to materials without incurring additional runtime draw
    // items as draw items for shaders are only created if the scene has a pass with a matching
    // DrawListTag. If your pipeline doesn't have a "lowEndForward" DrawListTag, no draw items
    // for this shader will be created.

    "Source" : "./StandardPBR_LowEndForward.azsl",

    "Definitions": ["QUALITY_LOW_END_TIER1=1", "OUTPUT_DEPTH=1"],

    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : true,
            "CompareFunc" : "GreaterEqual"
        },
        "Stencil" :
        {
            "Enable" : true,
            "ReadMask" : "0x00",
            "WriteMask" : "0xFF",
            "FrontFace" :
            {
                "Func" : "Always",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Replace"
            },
            "BackFace" :
            {
                "Func" : "Always",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Replace"
            }
        }
    },

    "ProgramSettings":
    {
        "EntryPoints":
        [
            {
                "name": "VertexShader",
                "type": "Vertex"
            },
            {
                "name": "PixelShader",
                "type": "Fragment"
            }
        ]
    },

    "DrawList" : "lowEndForward"
}
