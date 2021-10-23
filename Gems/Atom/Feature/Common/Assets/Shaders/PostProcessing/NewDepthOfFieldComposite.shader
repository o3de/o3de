{
    "Source" : "NewDepthOfFieldComposite.azsl",

    "DepthStencilState" : 
    {
        "Depth" : { "Enable" : false },
        "Stencil" : { "Enable" : false }
    },

    "BlendState" : {
        "Enable" : true,
        "BlendSource" : "AlphaSource",
        "BlendDest" : "AlphaSourceInverse",
        "BlendOp" : "Add"
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
    }
}
