
# Welcome to the AWS Core Resource Mapping Tool project!

## Setup aws config and credential
Resource mapping tool is using boto3 to interact with aws services:
 * Follow boto3
   [Configuration](https://boto3.amazonaws.com/v1/documentation/api/latest/guide/configuration.html) to setup default aws region.
 * Follow boto3
   [Credentials](https://boto3.amazonaws.com/v1/documentation/api/latest/guide/credentials.html) to setup default profile or credential keys.

Or follow **AWS CLI** configuration which can be reused by boto3 lib:
 * Follow
   [Quick configuration with aws configure](https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-quickstart.html#cli-configure-quickstart-config)

**In Progress** - Override default aws profile in resource mapping tool

## Python Environment Setup Options
### 1. Engine python environment
In order to use engine python environment, it requires to link Qt binaries for this tool.
Follow cmake instructions to configure your project, for example:

```
$ cmake -B <BUILD_FOLDER> -S . -G "Visual Studio 16 2019" -DLY_3RDPARTY_PATH=<PATH_TO_3RDPARTY> -DLY_PROJECTS=<PROJECT_NAME>
```

Build project with **AWSCore.Editor** target to generate required Qt binaries.
(Or use **Editor** target)

```
$ cmake --build <BUILD_FOLDER> --target AWSCore.Editor --config <CONFIG> -j <NUM_JOBS>
```

Launch resource mapping tool under engine root folder:

#### Windows
##### release mode
```
$ python\python.cmd Gems\AWSCore\Code\Tools\ResourceMappingTool\resource_mapping_tool.py --binaries_path <PATH_TO_BUILD_FOLDER>\bin\profile\AWSCoreEditorPlugins
```
##### debug mode
```
$ python\python.cmd debug Gems\AWSCore\Code\Tools\ResourceMappingTool\resource_mapping_tool.py --binaries_path <PATH_TO_BUILD_FOLDER>\bin\debug\AWSCoreEditorPlugins
```


### 2. Python virtual environment
This project is set up like a standard Python project. The initialization
process also creates a virtualenv within this project, stored under the `.env`
directory.  To create the virtualenv it assumes that there is a `python3`
(or `python` for Windows) executable in your path with access to the `venv`
package. If for any reason the automatic creation of the virtualenv fails,
you can create the virtualenv manually.

To manually create a virtualenv on MacOS and Linux:

```
$ python -m venv .env
```

Once the virtualenv is created, you can use the following step to activate your virtualenv.

```
$ source .env/bin/activate
```

If you are a Windows platform, you would activate the virtualenv like this:

```
% .env\Scripts\activate.bat
```

Once the virtualenv is activated, you can install the required dependencies.

```
$ pip install -r requirements.txt
```

#### 2.1 Launch Options
##### 2.1.1 Launch Resource Mapping Tool from python directly
At this point you can launch tool like other standard python project.

```
$ python resource_mapping_tool.py
```

##### 2.1.2 Launch Resource Mapping Tool from batch script/Editor
Update `resource_mapping_tool.cmd` with your virtualenv full path.

 * **VIRTUALENV_PATH**: Fill this variable with your virtualenv full path.

Then you can launch the resource mapping tool by running the batch script directly.

```
$ resource_mapping_tool.cmd
```

Or you can launch the resource mapping tool from menu action Cloud services/AWS Resource Mapping Tool...
