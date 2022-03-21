{
    "Source" : "./StandardMultilayerPBR_ForwardPass.azsl",

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
          "name": "ForwardPassVS",
          "type": "Vertex"
        },
        {
          "name": "ForwardPassPS",
          "type": "Fragment"
        }
      ]
    },
    
    "Supervariants":
    [
        {
            "Name": "",
            "AddBuildArguments": {
                "azslc": ["--no-alignment-validation"]
            }
        }
    ],

    "DrawList" : "forward"
}
