{
    "Source" : "./EnhancedPBR_ForwardPass.azsl",

    "Requirements": [
        "EvaluateUVs",
        // TODO: provide a non-parallax variant that doesn't require a world-space TBN
        "EvaluateWorldSpaceTBN",
        "EvaluatePixelDepth",
        "EvaluateEnhancedSurfaceAlphaClip"
    ],

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
          "name": "EnhancedPbr_ForwardPassVS",
          "type": "Vertex"
        },
        {
          "name": "EnhancedPbr_ForwardPassPS",
          "type": "Fragment"
        }
      ]
    },

    "DrawList" : "forwardWithSubsurfaceOutput"
} 
