# AWS Gem Automation Tests

## Prerequisites
1. Build the O3DE **Editor** and **AutomatedTesting.GameLauncher** in Profile.
2. Install the latest version of NodeJs.
3. AWS CLI is installed and AWS credentials are configured via [environment variables](https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-envvars.html) or [default profile](https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-files.html).
4. [AWS Cloud Development Kit (CDK)](https://docs.aws.amazon.com/cdk/v2/guide/getting_started.html#getting_started_install) is installed.

> Note: All the AWS Gems CDK applications have been updated to CDK v2. Ensure you set CDK_VERSION to the desired CDK v2 version. See https://docs.aws.amazon.com/cdk/api/versions.html for the latest CDK v2 version information.


> Support for the CDKv1 applications has been deprecated.

## Deploy CDK Applications
1. Go to the AWS IAM console and create an IAM role called o3de-automation-tests which adds your own account as a trusted entity and uses the "AdministratorAccess" permissions policy.
2. Copy the following deployment script to your engine root folder and update the `CDK_VERSION` to the desired CDK version:
    * Windows (Command Prompt)
        ```
            {engine_root}\scripts\build\Platform\Windows\deploy_cdk_applications.cmd
        ```
    * Linux
        ```
            {engine_root}/scripts/build/Platform/Linux/deploy_cdk_applications.sh
        ```
3. Open a new CLI window at the engine root and set the following environment variables:
    * Windows
        ```
            set O3DE_AWS_PROJECT_NAME=AWSAUTO
			set O3DE_AWS_DEPLOY_REGION=us-east-1
            set ASSUME_ROLE_ARN=arn:aws:iam::{your_aws_account_id}:role/o3de-automation-tests
            set COMMIT_ID=HEAD
            set CDK_VERSION=2.68.0 
            set AWS_EC2_METADATA_DISABLED=true
        ```
    * Linux
        ```
            export O3DE_AWS_PROJECT_NAME=AWSAUTO
            export O3DE_AWS_DEPLOY_REGION=us-east-1
            export ASSUME_ROLE_ARN=arn:aws:iam::{your_aws_account_id}:role/o3de-automation-tests
            export COMMIT_ID=HEAD
            export CDK_VERSION=2.68.0
            export AWS_EC2_METADATA_DISABLED=true
        ```
4. In the same CLI window, Deploy the CDK applications for AWS gems by running deploy_cdk_applications.cmd.
   
## Run Automation Tests
### CLI
1. In the same CLI window, run the following CLI command:
    * Windows
        ```
            python\python.cmd -m pytest {path_to_the_test_file} --build-directory {directory_to_the_profile_build}
        ```
    * Linux
        ```
            python/python.sh -m pytest {path_to_the_test_file} --build-directory {directory_to_the_profile_build}
        ```

The main tests files can be found in the ```AutomatedTesting\Gem\PythonTests\AWS``` directory at the following paths:

| Gem           | Test file                                      |
|---------------|------------------------------------------------|
| AWSCore       | core\test_aws_resource_interaction.py          |
| AWSMetrics    | aws_metrics\aws_metrics_automation_test.py     |
| AWSClientAuth | client_auth\aws_client_auth_automation_test.py |

### PyCharm
You can also run any specific automation test directly from [PyCharm](https://www.jetbrains.com/pycharm) by providing the "--build-directory" argument in the Run Configuration.

## Destroy CDK Applications
1. Copy the following destruction script to your engine root folder:
    * Windows
        ```
            {engine_root}\scripts\build\Platform\Windows\destroy_cdk_applications.cmd
        ```
    * Linux
        ```
            {engine_root}/scripts/build/Platform/Linux/destroy_cdk_applications.sh
        ```
2. In the same CLI window, destroy the CDK applications for AWS gems by running destroy_cdk_applications.cmd.
