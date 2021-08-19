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
          "name": "VelocityShockPropagation",
          "type": "Compute"
        }
      ]
    }   
}
