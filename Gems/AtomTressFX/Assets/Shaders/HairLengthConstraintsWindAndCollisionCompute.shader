{ 
    "Source" : "HairSimulationCompute.azsl",	

    "ProgramSettings":
    {
	  "EntryPoints":
      [
        {
          "name": "LengthConstriantsWindAndCollision",
          "type": "Compute"
        }
      ]
    },
    
    "disabledRHIBackends": ["webgpu"]   
}
