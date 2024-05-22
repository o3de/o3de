{
    "Source" : "SphereIntersection.azsl",
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
        }
    ]
}
