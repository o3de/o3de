{
    "Source" : "./MeshletsShadowMeshShader.azsl",

    // Phase 6 shadow-side DAG cut. Pairs the SHARED cluster-cull AS
    // (MeshletsCullAS.azsli) with the payload-driven MeshletsShadowPassMSCulled
    // entry. The AS runs in "cut-only" mode for shadows: the feature processor
    // zeroes m_doFrustumCull/m_doConeCull/m_doHiZCull on the shadow instance SRG
    // (a light must rasterize clusters outside the CAMERA frustum), so the only
    // rejection is the main-view DAG cut — shadow geometry matches the shaded cut
    // exactly. Split from MeshletsShadowMeshShader.shader for the same reason as
    // every other culled pair: the shipped no-cull PSO keeps plain DispatchMesh
    // semantics.
    //
    // Render state mirrors MeshletsShadowMeshShader.shader exactly (LessEqual +
    // depth bias — shadow maps are NOT reverse-Z here).
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
          "name": "MeshletsCullClustersAS",
          "type": "Amplification"
        },
        {
          "name": "MeshletsShadowPassMSCulled",
          "type": "Mesh"
        },
        {
          "name": "MeshletsShadowPassPS",
          "type": "Fragment"
        }
      ]
    },

    "DrawList" : "shadow"
}
