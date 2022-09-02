{
    "Source": "LightCullingTilePrepare.azsl",
    
    "ProgramSettings" : 
    {
        "EntryPoints":
        [
            {
                "name":  "MainCS",
                "type" : "Compute"
            }
        ] 
    },

    "Supervariants":
    [
        {
            "Name": "NoMSAA",
            "AddBuildArguments": {
                "azslc": ["--no-ms"]
            }
        }
    ]
}
