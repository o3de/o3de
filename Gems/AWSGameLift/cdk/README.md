# Welcome to O3DE GameLift Sample Project!
> This is the long term supported CDK v2 version of this template. If you have preexisting versions of this template
see the [CDK guidance](https://docs.aws.amazon.com/cdk/v2/guide/migrating-v2.html) about upgrading to CDK v2.

This is an optional CDKv2 application that provides two stacks:

  * A GameLift stack that contains all the GameLift resources required to host game servers
  * An optional support stack which is used to upload local build files and create GameLift builds

The `cdk.json` file tells the CDK Toolkit how to execute this application.

This project is set up like a standard Python project.  The initialization
process also creates a virtualenv within this project, stored under the `.env`
directory.  To create the virtualenv it assumes that there is a `python3`
(or `python` for Windows) (Python 3.10+) executable in your path with access to the `venv`
package. If for any reason the automatic creation of the virtualenv fails,
you can create the virtualenv manually.

See https://docs.aws.amazon.com/cdk/latest/guide/getting_started.html about for information about how to set up
the prerequisites for CDK development.

> Note: This stack is for CDK v2 (the latest CDK version, if you are working with CDKv1 stacks please use the CDKv1 version of this application).

## Make a virtual environment
To manually create a virtualenv on macOS and Linux:

```
$ python -m venv .venv
```

Once the virtualenv is created, you can use the following step to activate your virtualenv.

```
$ source .venv/bin/activate
```

If you are a Windows platform, you would activate the virtualenv like this:

```
% .venv\Scripts\activate.bat
```

Once the virtualenv is activated, you can install the required dependencies.

```
$ pip install -r requirements.txt
```

## Set environment variables or accept defaults

* `O3DE_AWS_DEPLOY_REGION`: The region to deploy the stacks into, will default to CDK_DEFAULT_REGION
* `O3DE_AWS_DEPLOY_ACCOUNT`: The account to deploy stacks into, will default to CDK_DEFAULT_ACCOUNT
* `O3DE_AWS_PROJECT_NAME`: The name of the O3DE project stacks should be deployed for will default to AWS-PROJECT

See https://docs.aws.amazon.com/cdk/v2/guide/environments.html for more information including how to pass parameters
to use for environment variables.

## Edit the sample fleet configurations

Before deploy the CDK application, you must update the fleet configurations defined in the
[sample fleet configurations](aws_gamelift/fleet_configurations.py) with project specific settings. 

You can either use an existing GameLift build id for creating a fleet or provide the local server package path 
for creating a new GameLift build.

You must also edit the `ec2_inbound_permissions` to include a valid ipaddress to allow connections from and to remove the
debug port setup which is not valid for your platform.

For windows builds remove the SSH port `22` in bound permission block, for Linux builds remove the RDP port `3389` block.

## Synthesize the application

At this point you can now synthesize the CloudFormation template for this code.

```
$ cdk synth
```

## Deploying the application

### Optional features
To create a game session queue using this CDK application, provide the following context variable 
when synthesize the CloudFormation template or deploy the application:

```
$ cdk deploy -c create_game_session_queue=true
```

### Double check fleet-configuration information
> Ensure you have set either `build_id` or `build_path` in [sample fleet configurations](aws_gamelift/fleet_configurations.py) .

You can also deploy a support stack which is used to upload local build files to S3 and provide GameLift access 
to the S3 objects when create GameLift builds. The local build path needs to be specified in the
[sample fleet configurations](aws_gamelift/fleet_configurations.py) if the feature is enabled. Otherwise, an existing
build id is required.

> Ensure you have set a valid ip address to `ip_range` in fleet_configurations.py to enable you to connect to your fleet.

For example, `0.0.0.0/32` where 0.0.0.0 should be replaced with the IP address of your development machine. Best practice is to not make RDP or SSH ports open to any machine but only allow connections from specific ip addresses.

### Deploy the AWS resources
```
$ cdk deploy -c upload-with-support-stack=true --all
```

You may need todo a one time bootstrap, once per account, per region. The CDK application will prompt you on this.

To add additional dependencies, for example other CDK libraries, just add
them to your `setup.py` file and rerun the `pip install -r requirements.txt`
command.

## Useful commands

 * `cdk ls`          list all stacks in the app
 * `cdk synth`       emits the synthesized CloudFormation template
 * `cdk deploy`      deploy this stack to your default AWS account/region
 * `cdk diff`        compare deployed stack with current state
 * `cdk docs`        open CDK documentation
 
## GameLift reference and best practise
https://docs.aws.amazon.com/gamelift/index.html

