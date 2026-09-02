{
    "Source" : "./MeshletsShadowMeshShader.azsl",

    // Shadow DAG cut: shared cull AS in cut-only mode (camera culls zeroed) --
    // shadow geometry matches the shaded cut. Render state mirrors
    // MeshletsShadowMeshShader.shader (LessEqual + bias -- shadows are not reverse-Z).
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
