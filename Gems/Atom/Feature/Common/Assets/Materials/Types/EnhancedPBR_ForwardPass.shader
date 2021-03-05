{
    "Source" : "./EnhancedPBR_ForwardPass.azsl",

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
            }
        }
    },

    "CompilerHints" : { 
        "DisableOptimizations" : true,
        "DxcGenerateDebugInfo" : true
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

    "DrawList" : "forward"
} 