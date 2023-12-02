# About the macro PRE_HLSL_2021

The macro PRE_HLSL_2021 was introduced since this bug:  https://github.com/o3de/o3de-extras/issues/625.

The workaround to the bug mentioned above was to use an older version of DXC 1.6.2112, instead of DXC 1.7.2308. DXC 1.7.2308 conforms to HLSL 2021 which doesn't support the ternary conditional operator (`(cond) ? true_statement : false_statement`) for vectors. HLSL 2021 only supports ternary conditional operator for scalars. Starting HLSL 2021, to achieve an equivalent behavior for ternary conditional operator for vectors, the `select()` intrinsic was introduced. See more details here:
https://devblogs.microsoft.com/directx/announcing-hlsl-2021/

This is why you'll find now code like this in some *.azsli or *.azsl files:
```hlsl
#ifdef PRE_HLSL_2021
    return (color.xyz < 0.04045) ? color.xyz / 12.92h : pow((color.xyz + real3(0.055, 0.055, 0.055)) / real3(1.055, 1.055, 1.055), 2.4);
#else
    return select((color.xyz < 0.04045), color.xyz / 12.92h, pow((color.xyz + real3(0.055, 0.055, 0.055)) / real3(1.055, 1.055, 1.055), 2.4));
#endif
```

By default the macro `PRE_HLSL_2021` is not defined, and the shaders will use the `and`, `or` and `select` intrinsics.

## When should the developer define the PRE_HLSL_2021 macro?
Defining the PRE_HLSL_2021 is required if the developer customizes the DXC executable to version pre-HLSL 2021 like DXC 1.6.2112.

The DXC executable can be customized via the Settings Registry Key:
`/O3DE/Atom/DxcOverridePath`.
Here is a customization example in a *.setreg file:
```json
{
    "O3DE": {
        "Atom": {
            "DxcOverridePath": "C:\\Users\\galib\\.o3de\\3rdParty\\packages\\DirectXShaderCompilerDxc-1.6.2112-o3de-rev1-windows\\DirectXShaderCompilerDxc\\bin\\Release\\dxc.exe"
        }
    }
}
```

## How to globally define the PRE_HLSL_2021 macro when compiling all shaders?
Customization of command line arguments for shader compilation tools is described here:
https://github.com/o3de/o3de/wiki/%5BAtom%5D-Developer-Guide:-Shader-Build-Arguments-Customization

In particular if you don't want to mess with engine files, in your game project:
1- Define an alternate location for `shader_build_options.json` using the registry key: `/O3DE/Atom/Shaders/Build/ConfigPath`.
2- Your new `shader_build_options.json` should be a copy of engine original located at `C:\GIT\o3de\Gems\Atom\Asset\Shader\Config\shader_build_options.json` with the addition of:
```json
    "Definitions": [ "PRE_HLSL_2021" ], // Needed when using DXC 1.6.2112
```
Complete example of customized shader_build_options.json:
```json
{
    "Definitions": [ "PRE_HLSL_2021" ], // Needed when using DXC 1.6.2112
    "AddBuildArguments": {
        "debug": false,
        "preprocessor": ["-C" // Preserve comments
              , "-+" // C++ mode
        ],
        "azslc": ["--full" // Always generate the *.json files with SRG and reflection info.
                , "--Zpr" // Row major matrices.
                , "--W1" // Warning Level 1
                , "--strip-unused-srgs" // Remove unreferenced SRGs.
                , "--root-const=128"
        ],
        "dxc": ["-Zpr" // Row major matrices.
        ],
        // The following apply for all Metal based platforms.
        "spirv-cross": ["--msl"
                      , "--msl-version"
                      , "20100"
                      , "--msl-invariant-float-math"
                      , "--msl-argument-buffers"
                      , "--msl-decoration-binding"
                      , "--msl-texture-buffer-native"
        ]
    }
}
```

Finally, if not done already you'd have to restart the AssetProcessor so it picks the changes from the Settings Registry.
