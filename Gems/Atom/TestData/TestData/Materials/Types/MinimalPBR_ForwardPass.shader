{
    "Source" : "./MinimalPBR_ForwardPass.azsl",

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
        "DisableOptimizations" : false
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MinimalPBR_MainPassVS",
          "type": "Vertex"
        },
        {
          "name": "MinimalPBR_MainPassPS",
          "type": "Fragment"
        }
      ]
    },

    "DrawList" : "forward"
}
