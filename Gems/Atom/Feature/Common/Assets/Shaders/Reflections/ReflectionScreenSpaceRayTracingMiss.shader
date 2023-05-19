{
    "Source" : "ReflectionScreenSpaceRayTracingMiss.azsl",
    "DrawList" : "RayTracing",

    "AddBuildArguments": 
    {
        "dxc": ["-fspv-target-env=vulkan1.2"]
    },

    "ProgramSettings":
    {
        "EntryPoints":
        [
            {
                "type": "RayTracing"
            }
        ]
    },

    "DisabledRHIBackends": ["metal"],

    "Supervariants":
    [
        {
            "Name": "",
            "RemoveBuildArguments": 
            {
                "azslc": ["--strip-unused-srgs"]
            }
        },
        {
            "Name": "NoMSAA",
            "AddBuildArguments": {
                "azslc": ["--no-ms"]
            },
            "RemoveBuildArguments" :
            {
                "azslc": ["--strip-unused-srgs"]
            }
        }
    ]
}
