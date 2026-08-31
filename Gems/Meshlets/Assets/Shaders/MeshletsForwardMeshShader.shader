{
    "Source" : "./MeshletsForwardMeshShader.azsl",

    // Mirrors MeshletsForwardPass.shader (and the standard MainPipeline forward
    // state) so the mesh-shader meshlets behave identically to standard meshes:
    // reverse-Z depth (GreaterEqual) + stencil Replace of the per-draw ref.
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
          "name": "MeshletsForwardPassMS",
          "type": "Mesh"
        },
        {
          "name": "MeshletsForwardMeshShaderPS",
          "type": "Fragment"
        }
      ]
    }
}
