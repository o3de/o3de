{ 
    "Source" : "HairSimulationCompute.azsl",

    "ProgramSettings":
    {
	  "EntryPoints":
      [
        {
          "name": "UpdateFollowHairVertices",
          "type": "Compute"
        }
      ]
    },
    
    "disabledRHIBackends": ["webgpu"]   
}
