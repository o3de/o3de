# Welcome to the AWS Metrics CDK Python project!

> This is the CDK v1 version of the sample project and is no longer receiving updates, please migrate to the CDKv2 version. 
CDK v1 entered maintenance on June 1, 2022, and will now receive only critical bug fixes and security patches. New features will be developed for CDK v2 exclusively.

This is an optional CDK application that provides a stack to build the metrics analytics pipeline. The deployed pipeline is simplified from the production ready AWS solution:
https://docs.aws.amazon.com/solutions/latest/game-analytics-pipeline/welcome.html.

AWS services used by this CDK application may not be available in all regions.

The `cdk.json` file tells the CDK Toolkit how to execute your app.

This project is set up like a standard Python project. The initialization
process also creates a virtualenv within this project, stored under the .env
directory.  To create the virtualenv it assumes that there is a `python3`
(or `python` for Windows) executable in your path with access to the `venv`
package. If for any reason the automatic creation of the virtualenv fails,
you can create the virtualenv manually. Please note that Python version 3.7.10 or higher is required to deploy this CDK application.

To manually create a virtualenv on macOS and Linux:

```
$ python -m venv .env
```

After the init process completes and the virtualenv is created, you can use the following
step to activate your virtualenv.

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

## Bootstrap the environment
An environment needs to be bootstrapped since this CDK application uses assets like a local directory that contains the handler code for the AWS Lambda functions.

Use the following CDK bootstrap command to bootstrap one or more AWS environments.

```
cdk bootstrap aws://ACCOUNT-NUMBER-1/REGION-1 aws://ACCOUNT-NUMBER-2/REGION-2 ...
```

See https://docs.aws.amazon.com/cdk/latest/guide/bootstrapping.html for more information about bootstrapping.

## Synthesize the project
At this point you can now synthesize the CloudFormation template for this code.

```
$ cdk synth
```

To add additional dependencies, for example other CDK libraries, just add
them to your `requirements.txt` file and rerun the `pip install -r requirements.txt`
command.

## Deploy the project
To deploy the CDK application, use the following CLI command:

```
$ cdk deploy
```

## Enable the optional batch processing feature
You can optionally enable the batch processing feature by specifying the context variable like below:

```
$ cdk synth -c batch_processing=true
$ cdk deploy -c batch_processing=true
```

This will deploy the AWS resources required by the batch processing feature and bring additional cost.

To disable the feature and remove related AWS resources, you need to empty the deployed S3 bucket manually and run the normal CDK CLI commands for updating the CDK application:

```
$ cdk synth
$ cdk deploy
```

## Useful commands

 * `cdk ls`          list all stacks in the app
 * `cdk synth`       emits the synthesized CloudFormation template
 * `cdk deploy`      deploy this stack to your default AWS account/region
 * `cdk diff`        compare deployed stack with current state
 * `cdk docs`        open CDK documentation
 
## Troubleshooting

See https://docs.aws.amazon.com/cdk/latest/guide/troubleshooting.html

