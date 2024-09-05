{
    "Source" : "ReflectionScreenSpaceRayTracingClosestHitProcedural.azsl",
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
                "type": "RayTracing",
                "name": "ClosestHitProcedural"
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
            "AddBuildArguments":
            {
                "azslc": ["--no-ms"]
            },
            "RemoveBuildArguments" :
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
        },
        {
            "Name": "NoFloat16NoMSAA",
            "AddBuildArguments":
            {
                "azslc": ["--no-ms"],
                "preprocessor": ["-DNO_FLOAT_16=1"]
            },
            "RemoveBuildArguments" :
            {
                "azslc": ["--strip-unused-srgs"]
            }
        }
    ]
}
