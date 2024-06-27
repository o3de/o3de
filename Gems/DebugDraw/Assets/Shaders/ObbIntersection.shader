{
    "Source" : "ObbIntersection.azsl",
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
                "name": "Intersection",
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
            "Name": "NoFloat16",
            "AddBuildArguments":
            {
                "preprocessor": ["-DNO_FLOAT_16=1"]
            },
            "RemoveBuildArguments":
            {
                "azslc": ["--strip-unused-srgs"]
            }
        }
    ]
}
