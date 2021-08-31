# AWS Gem Automation Tests

## Prerequisites
1. Build the O3DE Editor and AutomatedTesting.GameLauncher in Profile.
2. AWS CLI is installed and configured following [Configuration and Credential File Settings](https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-files.html).
3. [AWS Cloud Development Kit (CDK)](https://docs.aws.amazon.com/cdk/latest/guide/getting_started.html#getting_started_install) is installed.

## Deploy CDK Applications
1. Go to the AWS IAM console and create an IAM role called o3de-automation-tests which adds your own account as as a trusted entity and uses the "AdministratorAccess" permissions policy.
2. Copy {engine_root}\scripts\build\Platform\Windows\deploy_cdk_applications.cmd to your engine root folder.
3. Open a Command Prompt window at the engine root and set the following environment variables:  
   Set O3DE_AWS_PROJECT_NAME=AWSAUTO  
   Set O3DE_AWS_DEPLOY_REGION=us-east-1  
   Set ASSUME_ROLE_ARN="arn:aws:iam::{your_aws_account_id}:role/o3de-automation-tests"  
   Set COMMIT_ID=HEAD  
4. Deploy the CDK applications for AWS gems by running deploy_cdk_applications.cmd in the same Command Prompt window.
5. Edit AWS\common\constants.py to replace the assume role ARN with your own:  
   arn:aws:iam::{your_aws_account_id}:role/o3de-automation-tests
   
## Run Automation Tests
### CLI
Open a Command Prompt window at the engine root and run the following CLI command:  
python\python.cmd -m pytest {path_to_the_test_file} --build-directory {directory_to_the_profile_build}

### Pycharm
You can also run any specific automation test directly from Pycharm by providing the "--build-directory" argument in the Run Configuration.