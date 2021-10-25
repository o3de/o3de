
# Welcome to the AWS Core CDK Python project!

This is an optional CDK application that provides two stacks:

  * A core stack to use as the basis for a project's CDK application
  * An example stack with example resources that can be connected to ScriptBehavior samples in Core

The `cdk.json` file tells the CDK Toolkit how to execute your app.

This project is set up like a standard Python project.  The initialization
process also creates a virtualenv within this project, stored under the `.env`
directory.  To create the virtualenv it assumes that there is a `python3`
(or `python` for Windows) (Python 3.7+) executable in your path with access to the `venv`
package. If for any reason the automatic creation of the virtualenv fails,
you can create the virtualenv manually.

See https://docs.aws.amazon.com/cdk/latest/guide/getting_started.html about for information about how to set up
the prerequisites for CDK development.

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

## Set environment variables or accept defaults

* O3DE_AWS_DEPLOY_REGION*: The region to deploy the stacks into, will default to CDK_DEFAULT_REGION
* O3DE_AWS_DEPLOY_ACCOUNT*: The account to deploy stacks into, will default to CDK_DEFAULT_ACCOUNT
* O3DE_AWS_PROJECT_NAME*: The name of the O3DE project stacks should be deployed for will default to AWS-PROJECT

See https://docs.aws.amazon.com/cdk/latest/guide/environments.html for more information including how to pass parameters
to use for environment variables.

## Synthesize the project
At this point you can now synthesize the CloudFormation template for this code.

```
$ cdk synth
```

You may need todo a one time bootstrap, once per account, per region. The CDK application will prompt you on this.

To add additional dependencies, for example other CDK libraries, just add
them to your `setup.py` file and rerun the `pip install -r requirements.txt`
command.

## Optional Features
Server access logging is enabled by default. To disable the feature, use the following commands to synthesize and deploy this CDK application.

```
$ cdk synth -c disable_access_log=true --all
$ cdk deploy -c disable_access_log=true --all
```

See https://docs.aws.amazon.com/AmazonS3/latest/userguide/ServerLogs.html for more information about server access logging.

## Useful commands

 * `cdk ls`          list all stacks in the app
 * `cdk synth`       emits the synthesized CloudFormation template
 * `cdk deploy`      deploy this stack to your default AWS account/region
 * `cdk diff`        compare deployed stack with current state
 * `cdk docs`        open CDK documentation

## Troubleshooting

See https://docs.aws.amazon.com/cdk/latest/guide/troubleshooting.html
