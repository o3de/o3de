{
    "Source" : "./MeshletsMotionVectorMeshShader.azsl",

    // AS-culled motion PSO -- split so the shipped no-cull PSO keeps plain
    // DispatchMesh semantics. Render state mirrors the uncull .shader.
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
