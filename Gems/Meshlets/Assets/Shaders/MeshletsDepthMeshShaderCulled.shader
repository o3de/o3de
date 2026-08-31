{
    "Source" : "./MeshletsDepthMeshShader.azsl",

    // AS-culled depth prepass PSO (opt-in r_meshletsMsCullAS). SEPARATE .shader from
    // MeshletsDepthMeshShader.shader for the same reason the forward pair is split:
    // adding an Amplification stage to the default depth PSO would change the shipped
    // no-cull path's dispatch semantics. This PSO pairs the SHARED cluster-cull AS
    // (MeshletsCullAS.azsli — the same entry the culled forward PSO uses, so depth
    // and forward can never disagree on surviving clusters) with the payload-driven
    // MeshletsDepthPassMSCulled entry.
    //
    // Render state mirrors MeshletsDepthMeshShader.shader exactly (reverse-Z, depth
    // write on, no stencil).
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
