{
    "Source" : "./MeshletsForwardPass.azsl",

    // Mirrors the standard MainPipeline forward shader state
    // (ForwardPass_StandardLighting.shader.template) so meshlets behave
    // identically to standard meshes:
    //  - reverse-Z depth test (GreaterEqual)
    //  - stencil Replace writes the per-draw stencil ref (UseIBLSpecularPass |
    //    UseDiffuseGIPass = 0x83, set on the DrawItem) so the downstream
    //    Reflections and DiffuseGlobalIllumination fullscreen passes process
    //    meshlet pixels.
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

    "DrawList" : "forward",

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MeshletsForwardPassVS",
          "type": "Vertex"
        },
        {
          "name": "MeshletsForwardPassPS",
          "type": "Fragment"
        }
      ]
    }
}
