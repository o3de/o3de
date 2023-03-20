{
    // The editor mode mask shader writes out the visible entity of interest fragments to the red channel and the
    // visible + occluded fragments to the green channel. In order to achieve this, we turn off depth testing and
    // instead allow all entity of interest fragments to be rendered, whereupon we perform manual depth testing
    // in the fragment shader and update the relevent color channels of the mask texture with the closest depth
    // values of the entities of interest that pass the depth test. As we cannot read and write from and to the
    // mask texture in the same shader, we instead use the blending stage to accumulate the closest relevent 
    // fragments described into the mask texture.

    "Source" : "EditorModeMask.azsl",

    "DrawList" : "editormodeinterestmask",

    "DepthStencilState" : 
    { 
        "Depth" : { "Enable" : false }
    },

    // Overwrite the mask texture with any non-zero mask values from the fragment shader output
    "GlobalTargetBlendState" : 
    {
        "Enable" : true,
        "BlendSource" : "Zero",
        "BlendDest" : "Zero",
        "BlendOp" : "Maximum"
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MainVS",
          "type": "Vertex"
        },
        {
          "name": "MainPS",
          "type": "Fragment"
        }
      ]
    }    
}