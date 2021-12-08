
# Welcome to the AWS Core Resource Mapping Tool project!

## Setup aws config and credentials
The Resource Mapping Tool uses boto3 to interact with aws services:
 * Read boto3
   [Configuration](https://boto3.amazonaws.com/v1/documentation/api/latest/guide/configuration.html) to setup default aws region.
 * Read boto3
   [Credentials](https://boto3.amazonaws.com/v1/documentation/api/latest/guide/credentials.html) to setup default profile or credential keys.

Or follow **AWS CLI** configuration directions which can be reused by the boto3 lib:
 * Follow
   [Quick configuration with aws configure](https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-quickstart.html#cli-configure-quickstart-config)

## Python Environment Setup Options
### 1. Use Engine python environment (Including Editor)
1. In order to use the O3DE engine's python environment, this tool requires linking the Qt binaries.
Follow cmake instructions to configure your project, for example:
   ```
   $ cmake -B <BUILD_FOLDER> -S . -G "Visual Studio 16 2019" -DLY_PROJECTS=<PROJECT_NAME>
   ```

2. At this point, double check that the O3DE engine's python environment gets set up under *<ENGINE_ROOT_PATH>/python/runtime* directory

3. Build the project with **AWSCore.Editor** (or **AWSCore.ResourceMappingTool**, or **Editor**) target to generate the required Qt binaries.
   ```
   $ cmake --build <BUILD_FOLDER> --target AWSCore.Editor --config <CONFIG> -j <NUM_JOBS>
   ```

4. At this point, double check the Qt binaries have been generated under *<BUILD_FOLDER>/bin/<CONFIG_FOLDER>/AWSCoreEditorQtBin* directory

5. Launch the Resource Mapping Tool from the engine root folder:
   * Windows
      * release mode
      ```
      $ python\python.cmd Gems\AWSCore\Code\Tools\ResourceMappingTool\resource_mapping_tool.py --binaries_path <PATH_TO_BUILD_FOLDER>\bin\profile\AWSCoreEditorQtBin
      ```
      * debug mode
      ```
      $ python\python.cmd debug Gems\AWSCore\Code\Tools\ResourceMappingTool\resource_mapping_tool.py --binaries_path <PATH_TO_BUILD_FOLDER>\bin\debug\AWSCoreEditorQtBin
      ```
   * Linux
      * release mode
      ```
      $ python/python.sh Gems/AWSCore/Code/Tools/ResourceMappingTool/resource_mapping_tool.py --binaries_path <PATH_TO_BUILD_FOLDER>/bin/profile/AWSCoreEditorQtBin
      ```
      * debug mode
      ```
      $ python/python.sh Gems/AWSCore/Code/Tools/ResourceMappingTool/resource_mapping_tool.py --binaries_path <PATH_TO_BUILD_FOLDER>/bin/debug/AWSCoreEditorQtBin
      ```
      
* Note - the Editor is integrated with the same engine python environment to launch the Resource Mapping Tool. If it fails to launch the tool from the Editor, please follow above steps to make sure expected scripts and binaries are present.

### 2. Use a separate python virtual environment
This project is set up like a standard Python project. The initialization
process also creates a virtualenv within this project, stored under the `.env`
directory.  To create the virtualenv it assumes that there is a `python3`
(or `python` for Windows) executable in your path with access to the `venv`
package. If for any reason the automatic creation of the virtualenv fails,
you can create the virtualenv manually.

1. To manually create a virtualenv:
   * Windows
   ```
   $ python -m venv .env
   ```
   * Mac or Linux
   ```
   $ python3 -m venv .env
   ```

2. Once the virtualenv is created, you can use the following step to activate your virtualenv:
   * Windows
   ```
   % .env\Scripts\activate.bat
   ```
   * Mac or Linux
   ```
   $ source .env/bin/activate
   ```

3. Once the virtualenv is activated, you can install the required dependencies:
   * Windows
   ```
   $ pip install -r requirements.txt
   ```
   * Mac or Linux
   ```
   $ pip3 install -r requirements.txt
   ```

4. At this point you can launch tool like other standard python project.
   * Windows
   ```
   $ python resource_mapping_tool.py
   ```
   * Mac or Linux
   ```
   $ python3 resource_mapping_tool.py
   ```
## Tool Arguments
* `--binaries-path` **[Optional]** Path to QT Binaries necessary for PySide,
                    required if launching tool with engine python environment.
* `--config-path`   **[Optional]** Path to resource mapping config directory,
                    if not provided tool will use current directory.
* `--debug`         **[Optional]** Execute on debug mode to enable DEBUG logging level.
* `--log-path`      **[Optional]** Path to Resource Mapping Tool logging directory,
                    if not provided tool will store logging under tool source code directory.
* `--profile`       **[Optional]** Named AWS profile to use for querying AWS resources,
                    if not provided tool will use `default` aws profile.