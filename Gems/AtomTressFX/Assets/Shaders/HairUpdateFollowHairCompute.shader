{ 
    "Source" : "HairSimulationCompute.azsl",

    "CompilerHints" : { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : true
    },

    "ProgramSettings":
    {
	  "EntryPoints":
      [
        {
          "name": "UpdateFollowHairVertices",
          "type": "Compute"
        }
      ]
    }   
}
