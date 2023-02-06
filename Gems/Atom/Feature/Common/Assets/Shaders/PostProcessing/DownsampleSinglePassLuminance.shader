{
    "Source": "DownsampleSinglePassLuminance.azsl",

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MainCS",
          "type": "Compute"
        }
      ]
    },

    "AddBuildArguments": { 
      "dxc" : ["-fspv-target-env=vulkan1.1"] 
    },

    "Supervariants":
    [
        {
            "Name": "NoWave",
            "AddBuildArguments": {
                "preprocessor": ["-DSPD_NO_WAVE_OPERATIONS"]
            }
        }
    ]
}
