{
    "Source" : "./MeshletsForwardMeshShader.azsl",

    // Phase 5 AS/triangle cull (opt-in r_meshletsMsCullAS). SEPARATE .shader from
    // MeshletsForwardMeshShader.shader even though both compile the same .azsl source:
    // adding the Amplification stage to THAT shader would make every DispatchMesh call
    // through it invoke the AS unconditionally, changing the shipped default (no-cull)
    // mesh-shader path's behavior. This PSO adds Amplification + the payload-driven
    // MeshletsForwardPassMSCulled entry (per-triangle cull too) instead of
    // MeshletsForwardPassMS; the pixel shader is byte-for-byte shared.
    //
    // Mirrors MeshletsForwardMeshShader.shader's render state exactly (reverse-Z +
    // stencil Replace) so a mesh-shader-drawn cluster looks identical either way.
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
