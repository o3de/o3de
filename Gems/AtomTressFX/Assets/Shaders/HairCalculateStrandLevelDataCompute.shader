{ 
    "Source" : "HairCalculateStrandLevelData.azsl",
	
    "CompilerHints" : { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : true
    },

    "ProgramSettings":
    {
	  "EntryPoints":
      [
        {
          "name": "CalculateStrandLevelData",
          "type": "Compute"
        }
      ]
    }   
}
