{ 
    "Source" : "HairSimulationCompute.azsl",

    "ProgramSettings":
    {
	  "EntryPoints":
      [
        {
          "name": "LocalShapeConstraints",
          "type": "Compute"
        }
      ]
    },
    
    "disabledRHIBackends": ["webgpu"]   
}
