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
          "name": "InitializeSignedDistanceField",
          "type": "Compute"
        }
      ]
    }   
}
