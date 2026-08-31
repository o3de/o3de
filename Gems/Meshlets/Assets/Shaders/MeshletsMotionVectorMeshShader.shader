{
    "Source" : "./MeshletsMotionVectorMeshShader.azsl",

    // Mirrors MeshletsMotionVector.shader (the vertex-pull path): reverse-Z depth
    // test against the depth-prepass buffer, so only the frontmost meshlet
    // fragments emit motion.
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
          "name": "MeshletsMotionVectorMS",
          "type": "Mesh"
        },
        {
          "name": "MeshletsMotionVectorMeshPS",
          "type": "Fragment"
        }
      ]
    },

    // Same tag as the vertex-pull MeshletsMotionVector.shader, so MotionVectorPass
    // picks this DrawItem up with no .pass changes.
    "DrawList" : "motion"
}
