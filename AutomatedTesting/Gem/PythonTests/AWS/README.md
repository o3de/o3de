# AWS Gem Automation Tests

## Prerequisites
1. Build the O3DE Editor and AutomatedTesting.GameLauncher in Profile.
2. Install the latest version of NodeJs.
3. AWS CLI is installed and configured following [Configuration and Credential File Settings](https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-files.html).
4. [AWS Cloud Development Kit (CDK)](https://docs.aws.amazon.com/cdk/latest/guide/getting_started.html#getting_started_install) is installed.

## Deploy CDK Applications
1. Go to the AWS IAM console and create an IAM role called o3de-automation-tests which adds your own account as as a trusted entity and uses the "AdministratorAccess" permissions policy.
2. Copy the following deployment script to your engine root folde:
    * Windows
        {engine_root}\scripts\build\Platform\Windows\deploy_cdk_applications.cmd
    * Linux
        {engine_root}/scripts/build/Platform/Linux/deploy_cdk_applications.sh
3. Open a new CLI window at the engine root and set the following environment variables:
    * Windows
        ```
           Set O3DE_AWS_PROJECT_NAME=AWSAUTO
           Set O3DE_AWS_DEPLOY_REGION=us-east-1
           Set ASSUME_ROLE_ARN=arn:aws:iam::{your_aws_account_id}:role/o3de-automation-tests
           Set COMMIT_ID=HEAD
        ```
    * Linux
        ```
           export O3DE_AWS_PROJECT_NAME=AWSAUTO
           export O3DE_AWS_DEPLOY_REGION=us-east-1
           export ASSUME_ROLE_ARN=arn:aws:iam::{your_aws_account_id}:role/o3de-automation-tests
           export COMMIT_ID=HEAD
        ```
4. In the same CLI window, Deploy the CDK applications for AWS gems by running deploy_cdk_applications.cmd.
   
## Run Automation Tests
### CLI
In the same CLI window, run the following CLI command:  
    * Windows
    ```
        python\python.cmd -m pytest {path_to_the_test_file} --build-directory {directory_to_the_profile_build}
    ```
    * Linux
    ```
        python/python.sh -m pytest {path_to_the_test_file} --build-directory {directory_to_the_profile_build}
    ```

### Pycharm
You can also run any specific automation test directly from Pycharm by providing the "--build-directory" argument in the Run Configuration.

## Destroy CDK Applications
1. Copy the following destruction script to your engine root folder:
    * Windows
        {engine_root}\scripts\build\Platform\Windows\destroy_cdk_applications.cmd
    * Linux
        {engine_root}/scripts/build/Platform/Linux/destroy_cdk_applications.sh
2. In the same CLI window, destroy the CDK applications for AWS gems by running destroy_cdk_applications.cmd.
