# AZSLC Tests

These legacy Python tests run directly against an engine-built `azslc`.

The runner requires Python 3 and PyYAML. The engine's Python environment normally provides both. If a different environment is used, install PyYAML there first.

Build `azslc`, change to this directory, and pass the executable to `testapp.py`:

```
cmake --build <build-directory> --target Azslc --config Profile
cd <engine-root>/Code/Tools/ShaderCompiler/Tests
python testapp.py --silent \
    --compiler <build-directory>/bin/profile/azslc \
    --path Syntax Semantic Advanced Samples
```

On Windows, use the corresponding `azslc.exe` path. A successful complete run may include tests reported as `TODO`, but it must report no `FAIL` results.

Use `--path` to focus the run on a category or individual test:

```
python testapp.py --compiler <path-to-azslc> --path Syntax
python testapp.py --compiler <path-to-azslc> --path Semantic/AsError/deportedmethod-undeclared.azsl
python testapp.py --compiler <path-to-azslc> --path Advanced/emission-full.py
```

Paths are interpreted relative to this `Tests` directory. Omit `--silent` to show compiler output while diagnosing a failure.
