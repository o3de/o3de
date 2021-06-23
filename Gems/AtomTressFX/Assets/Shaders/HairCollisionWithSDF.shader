{ 
    "Source" : "HairCollisionWithSDF.azsl",
	
    "CompilerHints" : { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : true
    },

    "ProgramSettings":
    {
	   "EntryPoints":
      [
        {
          "name": "CollideHairVerticesWithSDF_forward",
          "type": "Compute"
        }
      ]
    }   
}
