{
    "Source" : "./MeshletsDepthMeshShader.azsl",

    // Mirrors MeshletsDepthPass.shader (the vertex-pull depth prepass): reverse-Z
    // depth test, depth write on (RHI default DepthWriteMask::All), NO stencil
    // block -- the prepass writes depth only; the forward item carries the stencil.
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
          "name": "MeshletsDepthPassMS",
          "type": "Mesh"
        },
        {
          "name": "MeshletsDepthPassPS",
          "type": "Fragment"
        }
      ]
    },

    // The Fragment entry is an EMPTY pixel shader (writes nothing -- the depth pass
    // binds no render targets), present only to satisfy azslc's input-assembly
    // reflection: a Mesh entry contributes no IA function data, so a mesh-ONLY
    // shader fails the build with "The number of valid shader entry functions ...
    // was 0!". See the comment on MeshletsDepthPassPS in the .azsl.

    // Standard Atom depth-prepass tag -- same tag as the vertex-pull
    // MeshletsDepthPass.shader, so the early DepthPrePass picks this DrawItem up
    // with no gem-private pass and no .pass/.azasset changes.
    "DrawList" : "depth"
}
