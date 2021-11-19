# AWS Gem Automation Tests

## Prerequisites
1. Build the O3DE Editor and AutomatedTesting.GameLauncher in Profile.
2. AWS CLI is installed and configured following [Configuration and Credential File Settings](https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-files.html).
3. [AWS Cloud Development Kit (CDK)](https://docs.aws.amazon.com/cdk/latest/guide/getting_started.html#getting_started_install) is installed.

## Deploy CDK Applications
1. Go to the AWS IAM console and create an IAM role called o3de-automation-tests which adds your own account as as a trusted entity and uses the "AdministratorAccess" permissions policy.
2. Copy {engine_root}\scripts\build\Platform\Windows\deploy_cdk_applications.cmd to your engine root folder.
3. Open a new Command Prompt window at the engine root and set the following environment variables:
```
   Set O3DE_AWS_PROJECT_NAME=AWSAUTO
   Set O3DE_AWS_DEPLOY_REGION=us-east-1
   Set O3DE_AWS_DEPLOY_ACCOUNT={your_aws_account_id}
   Set ASSUME_ROLE_ARN=arn:aws:iam::{your_aws_account_id}:role/o3de-automation-tests
   Set COMMIT_ID=HEAD
```
4. In the same Command Prompt window, Deploy the CDK applications for AWS gems by running deploy_cdk_applications.cmd.
   
## Run Automation Tests
### CLI
In the same Command Prompt window, run the following CLI command:  
python\python.cmd -m pytest {path_to_the_test_file} --build-directory {directory_to_the_profile_build}

### Pycharm
You can also run any specific automation test directly from Pycharm by providing the "--build-directory" argument in the Run Configuration.

## Destroy CDK Applications
1. Copy {engine_root}\scripts\build\Platform\Windows\destroy_cdk_applications.cmd to your engine root folder.
2. In the same Command Prompt window, destroy the CDK applications for AWS gems by running destroy_cdk_applications.cmd.