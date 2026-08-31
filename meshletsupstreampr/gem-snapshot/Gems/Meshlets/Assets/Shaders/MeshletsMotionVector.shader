{
    "Source" : "./MeshletsMotionVector.azsl",

    // Mirrors MeshMotionVector.shader.template: reverse-Z depth test against the
    // depth-prepass buffer (so only frontmost meshlet fragments emit motion).
    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : true,
            "CompareFunc" : "GreaterEqual"
        }
    },

    "DrawList" : "motion",

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MeshletsMotionVectorVS",
          "type": "Vertex"
        },
        {
          "name": "MeshletsMotionVectorPS",
          "type": "Fragment"
        }
      ]
    }
}
