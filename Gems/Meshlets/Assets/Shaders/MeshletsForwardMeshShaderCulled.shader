{
    "Source" : "./MeshletsForwardMeshShader.azsl",

    // AS+triangle-cull forward PSO -- separate .shader so the shipped no-cull PSO
    // keeps plain DispatchMesh semantics. Render state mirrors the uncull .shader.
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
          "name": "MeshletsCullClustersAS",
          "type": "Amplification"
        },
        {
          "name": "MeshletsForwardPassMSCulled",
          "type": "Mesh"
        },
        {
          "name": "MeshletsForwardMeshShaderPS",
          "type": "Fragment"
        }
      ]
    }
}
