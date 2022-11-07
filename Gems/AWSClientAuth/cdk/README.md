# Welcome to AWSClientAuth CDK Python project!

> This is the long term supported AWS Cloud Development Kit (CDK) v2 version of this template. If you have preexisting versions of this template
see the [CDK guidance](https://docs.aws.amazon.com/cdk/v2/guide/migrating-v2.html) about upgrading to CDK v2.

This is an optional CDKv2 application to use as a starting point when working with the ClientAuth gem. It will create 
a stack with Cognito Identity and Client pools.

> If you are working with pre-existing CDK v1 stacks please use the CDKv1 version of this application.

## Install prerequisites for CDK
https://docs.aws.amazon.com/cdk/v2/guide/getting_started.html#getting_started_prerequisites for information about how to set up
the prerequisites for CDK development.

## Install CDK
https://docs.aws.amazon.com/cdk/v2/guide/getting_started.html#getting_started_install

The `cdk.json` file tells the CDK Toolkit how to execute your app.

### Setup Python

This project is set up like a standard Python project. You can use either a [python virtual environment](https://docs.aws.amazon.com/cdk/v2/guide/work-with-cdk-python.html) or use python interpreter from O3DE to setup python dependencies ie:
```
set PATH="..\..\..\python\runtime\python-3.10.5-rev1-windows\python\";%PATH%
```

Once the python and pip are set up, you can install the required dependencies. To setup with O3DE's python interpreter use:
```
# Run from cdk folder in AWSClientAuth gem
..\..\..\python\pip.cmd install -r requirements.txt
```

### Configure the AWSClientAuth CDK application

CD in the `cdk` folder and set the following [environment variables](https://docs.aws.amazon.com/cdk/v2/guide/environments.html):
```
set O3DE_AWS_DEPLOY_REGION="us-west-2"
set O3DE_AWS_DEPLOY_ACCOUNT=""
set O3DE_AWS_PROJECT_NAME="AWSIProject"

If you want to add 3rd party providers fill values in utils/constant.py
```
List stacks 
```
cdk ls
```

At this point you can now synthesize the CloudFormation template for this code.

```
cdk synth <stackname>
```
Deploy stacks. Note passed parameters. Deploy will throw error for non-optional parameters.
```
cdk deploy <stackname> --profile <profile-name>
```

To add additional dependencies, for example other CDK libraries, just add them to your requirements.txt file and rerun the `..\..\..\python\pip.cmd install -r .\Gems\AWSClientAuth\cdk\requirements.txt` command.


## Update Authorization Permissions
To give permissions to call AWS resources, please update CognitoIdentityPoolRole class with correct policy statements.

An example IAM permission policy is provided to grant both authenticated and unauthenticated the permission to list S3 buckets in the project.
However, it is expected that developers replace these permissions with those required by your users to use your resources.

## Useful commands

 * `cdk ls`          list all stacks in the app
 * `cdk synth`       emits the synthesized CloudFormation template
 * `cdk deploy`      deploy this stack to your default AWS account/region
 * `cdk diff`        compare deployed stack with current state
 * `cdk docs`        open CDK documentation

Enjoy!
