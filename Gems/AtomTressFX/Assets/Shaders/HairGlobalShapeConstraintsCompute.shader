{ 
    "Source" : "HairSimulationCompute.azsl",	

    "ProgramSettings":
    {
	  "EntryPoints":
      [
        {
          "name": "IntegrationAndGlobalShapeConstraints",
          "type": "Compute"
        }
      ]
    },
    
    "disabledRHIBackends": ["webgpu"]   
}
