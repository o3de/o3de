{ 
    "Source" : "HairSimulation.azsl",
	
    "CompilerHints" : { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : true
    },

    "ProgramSettings":
    {
	   "EntryPoints":
      [
        {
          "name": "SkinHairVerticesOnly",
          "type": "Compute"
        }
      ]
    }   
}
