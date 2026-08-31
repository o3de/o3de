{
    "Source" : "./MeshletsShadowMeshShader.azsl",

    // Mirrors MeshletsShadowPass.shader (the vertex-pull shadow pass) exactly:
    // LessEqual depth test (shadow maps are NOT reverse-Z here) plus the same
    // depth bias, so the mesh-shader path produces the same acne/peter-panning
    // characteristics as the path it replaces.
    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : true,
            "CompareFunc" : "LessEqual"
        }
    },

    "RasterState" :
    {
        "depthBias" : "10",
        "depthBiasSlopeScale" : "4"
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MeshletsShadowPassMS",
          "type": "Mesh"
        },
        {
          "name": "MeshletsShadowPassPS",
          "type": "Fragment"
        }
      ]
    },

    // The Fragment entry is an EMPTY pixel shader, present only to satisfy azslc's
    // input-assembly reflection — see the comment on MeshletsShadowPassPS in the
    // .azsl, and the longer version in MeshletsDepthMeshShader.azsl.

    // Same tag as the vertex-pull MeshletsShadowPass.shader, so every shadowmap
    // pass (cascades, projected) picks this DrawItem up with no .pass changes.
    "DrawList" : "shadow"
}
