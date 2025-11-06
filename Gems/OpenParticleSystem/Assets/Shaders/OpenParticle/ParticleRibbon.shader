{
    "Source" : "ParticleRibbon.azsl",

    "DepthStencilState" : {
        "Depth" : { "Enable" : true, "WriteMask" : "Zero", "CompareFunc" : "GreaterEqual" }
    },

    "GlobalTargetBlendState": {
      "Enable": true,
      "BlendSource": "AlphaSource",
      "BlendDest": "AlphaSourceInverse",
      "BlendOp": "Add"
  },

    "RasterState": {
        "CullMode": "None"
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MainVS",
          "type": "Vertex"
        },
        {
          "name": "MainPS",
          "type": "Fragment"
        }
      ]
    },

    "DrawList" : "transparent"
}
