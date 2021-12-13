{
    // Note: "LowEnd" shaders are for supporting the low end pipeline
    // These shaders can be safely added to materials without incurring additional runtime draw
    // items as draw items for shaders are only created if the scene has a pass with a matching
    // DrawListTag. If your pipeline doesn't have a "lowEndForward" DrawListTag, no draw items
    // for this shader will be created.

    "Source" : "./StandardPBR_LowEndForward.azsl",

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

    "CompilerHints" : { 
        "DisableOptimizations" : false
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "StandardPbr_ForwardPassVS",
          "type": "Vertex"
        },
        {
          "name": "StandardPbr_ForwardPassPS",
          "type": "Fragment"
        }
      ]
    },

    "DrawList" : "lowEndForward"
}
