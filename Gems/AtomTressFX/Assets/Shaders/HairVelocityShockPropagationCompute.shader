{ 
    "Source" : "HairVelocityShockPropagation.azsl",
	
    "CompilerHints" : { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : true
    },

    "ProgramSettings":
    {
	   "EntryPoints":
      [
        {
          "name": "VelocityShockPropagation",
          "type": "Compute"
        }
      ]
    }   
}
