{
    "Source" : "./MeshletsDepthMeshShader.azsl",

    // Two-pass PASS 2: the culled depth MS tagged for the injected late pass
    // (after this frame's HiZ reduce, visMode 2 -- disoccluded clusters complete
    // the depth). Depth state matches the prepass.
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
