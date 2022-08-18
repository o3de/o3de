{
  "Source" : "./Eye.azsl",

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
        "name": "EyeVS",
        "type": "Vertex"
      },
      {
        "name": "EyePS",
        "type": "Fragment"
      }
    ]
  },

  "DrawList" : "forwardWithSubsurfaceOutput"
} 
