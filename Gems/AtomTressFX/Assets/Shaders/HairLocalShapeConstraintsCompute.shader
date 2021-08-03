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
          "name": "LocalShapeConstraints",
          "type": "Compute"
        }
      ]
    }   
}
