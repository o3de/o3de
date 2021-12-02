{
    "Source": "LightCullingTilePrepare.azsl",

    "CompilerHints":
    {
        "DisableOptimizations":false
    },
    
    "ProgramSettings" : 
    {
        "EntryPoints":
        [
            {
                "name": "MainCS",
                "type" : "Compute"
            }
        ] 
    },

    "Supervariants":
    [
        {
            "Name": "NoMSAA",
            "PlusArguments": "--no-ms",
            "MinusArguments": ""
        }
    ]
}
