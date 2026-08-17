# AZSLC

AZSLC is the front-end compiler for Atom's shader build pipeline. It transpiles Amazon Shading Language (AZSL) source into HLSL and generates reflection data for resource bindings, shader options, and other shader metadata.

The host-tools build provides two internal targets:

- `AZ::ShaderCompiler` is the static compiler implementation.
- `AZ::Azslc` is the command-line executable and produces `azslc`.

## Building

Configure a build directory, then build the command-line target:

```
cmake --build <build-directory> --target Azslc --config Profile
```

The executable is written to `<build-directory>/bin/profile/azslc`. Building Atom Shader Builder also builds and stages the executable under `Builders/AZSLc`.

## Grammar

The generated ANTLR C++ sources are checked in. With Java and PowerShell available, regenerate them from the engine root:

```
pwsh Code/Tools/ShaderCompiler/Source/Grammar/Generate.ps1
```

The script downloads ANTLR when it is not cached locally before invoking it.
Commit grammar and generated-source changes together.
