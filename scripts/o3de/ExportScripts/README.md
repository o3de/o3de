# Project Exporter Scripts

The project export scripts provides a simplified workflow to export a release version of a project that can be launched independently of the O3DE engine on the desired target environment or platform.

> Note: The scripts in this folder provide commands to build and generate a generic exported project. These scripts are meant to be run through the `export-project` command through the `o3de` command line interface, and is not meant to be ran directly.*

## export_source_built_project.py

This script will generate bundles and launcher executables, and optionally archive them into a compressed folder. Since it can optionally build the required tools for asset processing and asset bundling, it is only applicable to a source engine build, not an SDK installed engine. 

> Note: Seed lists are a way to target which assets are bundled during this process. Refer to the [Bundling project assets](https://www.docs.o3de.org/docs/user-guide/packaging/asset-bundler/bundle-assets-for-release/) instructions for more details.

### Example Execution
The following example assumes the location of the O3DE (source) engine, the path to the project, and the seed list for asset bundling are all available and set before hand. Adjust accordingly to match your specific project.

**Windows**
```
SET O3DE_PATH="%HOMEPATH%\O3DE\Engines\o3de"
SET O3DE_PROJECT_PATH="%HOMEPATH%\O3DE\Projects\NewProject"
SET O3DE_PROJECT_SEEDLIST=%O3DE_PROJECT_PATH%/Assets/seedlist.seed
SET OUTPUT_PATH="%HOMEPATH%\O3DE\Projects\NewProjectPackages"

%O3DE_PATH%\scripts\o3de.bat export-project -es ExportScripts\export_source_built_project.py --project-path %O3DE_PROJECT_PATH% \
 --log-level INFO -assets --build-tools --config release --archive-output zip --seedlist %O3DE_PROJECT_SEEDLIST% -out %OUTPUT_PATH%
```


**Linux**
```
export O3DE_PATH=$HOME/o3de
export O3DE_PROJECT_PATH=$HOME/O3DE/NewProject
export O3DE_PROJECT_SEEDLIST=$O3DE_PROJECT_PATH/Assets/seedlist.seed
export OUTPUT_PATH=$HOME/NewProjectPackages

$O3DE_PATH/scripts/o3de.sh export-project -es ExportScripts/export_source_built_project.py  --project-path $O3DE_PROJECT_PATH \
--log-level INFO -assets --build-tools --config release --archive-output zip --seedlist $O3DE_PROJECT_SEEDLIST -out $OUTPUT_PATH
```

<br/>
For a list of all options, use the `--script-help` argument for the export script

**Windows**
```
%O3DE_PATH%\scripts\o3de.bat export-project --export-script ExportScripts/export_source_built_project.py --script-help
```

**Linux**
```
$O3DE_PATH/scripts/o3de.sh export-project --export-script ExportScripts/export_source_built_project.py --script-help
```

## Customizing the project export script
The `export_source_built_script.py` is a general purpose export script that exposes steps that are needed to generate
a bundled package for the project. It is possible to write a custom script that could perform additional tasks for the
packaging of the project, or it can hard-code known values for the project, such as asset seed lists, files to include,
etc. By launching the script through the `export-project` command, the export scripts are provided access to the o3de
python package as well as a `O3DEScriptExportContext` global instance called `o3de_context` which provides to the script
the following:

|property|description|
---------|-----------|
|`export_script_path`| The absolute path to the export script being run|
|`project_path`| The absolute path to the project being exported|
|`engine_path`| The absolute path to the engine that the project is built with|
|`args`|  A list of the CLI arguments that were unparsed, and passed through for further processing, if necessary.|
|`cmake_additional_build_args`|A list additional CLI arguments that were unparsed, and passed through for further processing, if necessary.|
|`project_name`|The name of the project at the project path|


The custom script is provided the export-script specific arguments through the `args` property of the `o3de_context`, and the script is expected parse
these args to extract the export options.

> note: It is required to provide a handler for `--script-help` since the arg parser's handling of `--help` will display the help just for the `export-script` command.
>

The custom script will be processed by passing in its path to the `--export-script` argument.
