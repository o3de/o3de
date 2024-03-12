# Welcome to AWSClientAuth CDK Python project!

> This is a legacy CDK v1 application and is provided for backwards compatibility only. 
The CDK has migrated to a new long term version CDK v2. See the CDK documentation for guidance of how to upgrade.

This is an optional CDKv1 application to use as a starting point when working with the ClientAuth gem. It will create 
a stack with Cognito Identity and Client pools.

## Install prerequisites for CDK
https://docs.aws.amazon.com/cdk/latest/guide/getting_started.html#getting_started_prerequisites

## Install cdk
https://docs.aws.amazon.com/cdk/latest/guide/getting_started.html#getting_started_install

The `cdk.json` file tells the CDK Toolkit how to execute your app.

Run from cdk directory within gem. 

Optional set up python path to LY Python
```
set PATH="..\..\..\python\";%PATH%
```

This project is set up like a standard Python project.  Use python interpreter from Open3d to setup python dependencies
Once the python and pip are set up, you can install the required dependencies.

```
# Run from cdk folder in AWSClientAuth gem
..\..\..\python\pip.cmd install -r requirements.txt
```

Set variables
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

To add additional dependencies, for example other CDK libraries, just add
them to your requirements.txt file and rerun the `..\..\..\Lumberyard\python\pip.cmd install -r .\Gems\AWSClientAuth\cdk\requirements.txt`
command.


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
