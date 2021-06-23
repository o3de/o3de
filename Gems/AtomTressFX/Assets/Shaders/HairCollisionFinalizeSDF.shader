{ 
    "Source" : "HairCollisionPrepareSDF.azsl",
	
    "CompilerHints" : { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : true
    },

    "ProgramSettings":
    {
	   "EntryPoints":
      [
        {
          "name": "FinalizeSignedDistanceField",
          "type": "Compute"
        }
      ]
    }   
}
