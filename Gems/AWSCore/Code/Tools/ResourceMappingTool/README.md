
# Welcome to the AWS Core Resource Mapping Tool project!

This project is set up like a standard Python project.  The initialization
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

## Launch Options
### 1. Launch Resource Mapping Tool from python directly
At this point you can launch tool like other standard python project.

```
$ python resource_mapping_tool.py
```

### 2. Launch Resource Mapping Tool from batch script/Editor
Update `resource_mapping_tool.cmd` with your virtualenv full path.

 * **VIRTUALENV_PATH**: Fill this variable with your virtualenv full path.

Then you can launch the resource mapping tool by running the batch script directly.

```
$ resource_mapping_tool.cmd
```

Or you can launch the resource mapping tool from menu action Cloud services/AWS Resource Mapping Tool...
