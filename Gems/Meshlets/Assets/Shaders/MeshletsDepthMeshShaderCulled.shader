{
    "Source" : "./MeshletsDepthMeshShader.azsl",

    // AS-culled depth PSO -- split so the shipped no-cull PSO keeps plain
    // DispatchMesh semantics; shares the cull AS with forward, so depth and
    // forward can never disagree on survivors. Render state mirrors the uncull .shader.
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
          "name": "MeshletsDepthPassMSCulled",
          "type": "Mesh"
        },
        {
          "name": "MeshletsDepthPassPS",
          "type": "Fragment"
        }
      ]
    },

    "DrawList" : "depth"
}
