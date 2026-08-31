{
    "Source" : "./MeshletsMotionVectorMeshShader.azsl",

    // AS-culled motion vector PSO (opt-in r_meshletsMsCullAS). Split from
    // MeshletsMotionVectorMeshShader.shader for the same reason as the forward and
    // depth pairs -- the shipped no-cull PSO must keep its plain DispatchMesh
    // semantics. Pairs the SHARED cluster-cull AS (MeshletsCullAS.azsli) with the
    // payload-driven MeshletsMotionVectorMSCulled entry.
    //
    // Render state mirrors MeshletsMotionVectorMeshShader.shader exactly.
    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : true,
            "CompareFunc" : "GreaterEqual"
        }
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MeshletsCullClustersAS",
          "type": "Amplification"
        },
        {
          "name": "MeshletsMotionVectorMSCulled",
          "type": "Mesh"
        },
        {
          "name": "MeshletsMotionVectorMeshPS",
          "type": "Fragment"
        }
      ]
    },

    "DrawList" : "motion"
}
