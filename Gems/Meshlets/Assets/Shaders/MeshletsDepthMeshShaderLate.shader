{
    "Source" : "./MeshletsDepthMeshShader.azsl",

    // Two-pass occlusion, PASS 2 (opt-in r_meshletsTwoPassOcclusion): same AS +
    // payload depth MS as MeshletsDepthMeshShaderCulled.shader, but tagged for the
    // gem-injected MeshletsLateDepthPass, which runs AFTER this frame's HiZ pyramid
    // is reduced. Its instance SRG runs the AS in visMode 2: only clusters pass 1
    // skipped are tested (against THIS frame's pyramid) and the disoccluded
    // survivors complete the depth buffer — removing the one-frame disocclusion pop
    // and guaranteeing the pyramid is never built from a feedback-culled depth.
    //
    // Depth state matches the depth prepass exactly (reverse-Z, write on).
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

    // Gem-private tag: routed ONLY to MeshletsLateDepthPass (injected after
    // GpuCullAndDrawPass), never to the standard early DepthPrePass.
    "DrawList" : "meshletslatedepth"
}
