{
    "Source" : "./MeshletsDepthPass.azsl",

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
          "name": "MeshletsDepthPassVS",
          "type": "Vertex"
        }
      ]
    },

    // Standard Atom depth-prepass tag. The early DepthPrePass renders this
    // DrawItem into the main depth buffer that FullscreenShadow, SSAO,
    // reflections, and the forward depth-test all read -- so meshlets occlude
    // and receive shadows identically to standard meshes. (Was the gem-private
    // "MeshletsDepthDrawList", which only fed a late gem pass injected after
    // OpaquePass; that ran too late for the depth-consuming effects, making
    // meshlets look translucent and letting shadows pass through.)
    "DrawList" : "depth"
}
