# Project Exporter Scripts

The project export scripts provides a simplified workflow to export a release version of a project that can be launched independently of the O3DE engine on the desired target environment or platform.

*The scripts in this folder provide commands to build and generate a generic exported project. These scripts are meant to be run through the `export-project` command through the `o3de` command line interface, and is not meant to be ran directly.*

### export_source_built_project.py

This script will generate bundles and launcher executables, and optionally archive them into a compressed folder. Since it can optionally build the required tools for asset processing and asset bundling, it is only applicable to a source engine build, not an SDK installed engine. 

You must have seed list(s) generated for the project's assets before proceeding. Refer to the [Bundling project assets](https://www.docs.o3de.org/docs/user-guide/packaging/asset-bundler/bundle-assets-for-release/) instructions for more details.

#### Example Execution
The following example assumes the location of the O3DE (source) engine, the path to the project, and the seed list for asset bundling are all available and set before hand. Adjust accordingly to match your specific project.

**Linux**
```
EXPORT O3DE_PATH=$HOME/o3de
EXPORT O3DE_PROJECT_PATH=$HOME/O3DE/NewProject
EXPORT O3DE_PROJECT_SEEDLIST=$O3DE_PROJECT_PATH/Assets/seedlist.seed
EXPORT OUTPUT_PATH=$HOME/NewProjectPackages

$O3DE_PATH/scripts/o3de.sh export-project --export-scripts ExportScripts/export_source_built_project.py 
 --project-path $O3DE_PROJECT_PATH --log-level INFO --output-path %OUTPUT_PATH% --build-tools --config release --archive-output zip \
 --seedlist $O3DE_PROJECT_SEEDLIST
```

**Windows**
```
SET O3DE_PATH="%HOMEPATH%\O3DE\Engines\o3de"
SET O3DE_PROJECT_PATH="%HOMEPATH%\O3DE\Projects\NewProject"
SET O3DE_PROJECT_SEEDLIST=%O3DE_PROJECT_PATH%/Assets/seedlist.seed
SET OUTPUT_PATH="%HOMEPATH%\O3DE\Projects\NewProjectPackages"

%O3DE_PATH%/scripts/o3de.bat export-project --export-scripts ExportScripts/export_source_built_project.py 
 --project-path %O3DE_PROJECT_PATH% --log-level INFO --output-path %OUTPUT_PATH% --build-tools --config release --archive-output zip \
 --seedlist %O3DE_PROJECT_SEEDLIST%
```


> Note: You can pass through build specific arguments to the underlying cmake build process with a trailing `/` argument. </br> 
> For example, you can pass visual studio specific arguments by adding `/ -- /m` to the argument list above to request </br>
> Parallel build tasks.

For a list of all options, use the `--script-help` argument for the export script

**Linux**
```
$O3DE_PATH/scripts/o3de.sh export-project --export-scripts ExportScripts/export_source_built_project.py --script-help 
```

**Windows**
```
%O3DE_PATH%/scripts/o3de.bat export-project --export-scripts ExportScripts/export_source_built_project.py --script-help 
```
