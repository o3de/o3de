{
    "Source" : "./TerrainPBR_ForwardPass.azsl",

    "CompilerHints" :
    { 
        "DisableOptimizations" : false,
        "GenerateDebugInfo" : false
    },

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

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "TerrainPBR_MainPassVS",
          "type": "Vertex"
        },
        {
          "name": "TerrainPBR_MainPassPS",
          "type": "Fragment"
        }
      ]
    },

    "DrawList" : "forward",
    "DisabledRHIBackends": ["metal"]
}
