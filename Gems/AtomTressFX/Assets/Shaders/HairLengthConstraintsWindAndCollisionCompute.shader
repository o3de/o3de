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
          "name": "LengthConstriantsWindAndCollision",
          "type": "Compute"
        }
      ]
    }   
}
