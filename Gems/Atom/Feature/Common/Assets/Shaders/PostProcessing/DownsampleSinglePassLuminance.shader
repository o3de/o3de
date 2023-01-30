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
