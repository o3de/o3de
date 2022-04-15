#!/bin/bash

# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# Deploy the CDK applications for AWS gems (Linux only)
# Prerequisites:
# 1) Node.js is installed
# 2) Node.js version >= 10.13.0, except for versions 13.0.0 - 13.6.0. A version in active long-term support is recommended.

SOURCE_DIRECTORY=$PWD
PATH=$SOURCE_DIRECTORY/python:$PATH
GEM_DIRECTORY=$SOURCE_DIRECTORY/Gems

DestroyCDKApplication()
{
# Destroy the CDK application for a specific AWS gem
GEM_NAME=$1
echo [cdk_destruction] Destroy the CDK application for the $GEM_NAME gem
pushd $GEM_DIRECTORY/$GEM_NAME/cdk

# Revert the CDK application code to a stable state using the provided commit ID
if ! git checkout $COMMIT_ID -- .;
then
    echo [git_checkout] Failed to checkout the CDK application for the $GEM_NAME gem using commit ID $COMMIT_ID
    popd
    return 1
fi

# Install required packages for the CDK application
if ! python -m pip install -r requirements.txt;
then
    echo [cdk_destruction] Failed to install required packages for the $GEM_NAME gem
    popd
    return 1
fi

# Destroy the CDK application
if ! cdk destroy --all -f;
then
    echo [cdk_destruction] Failed to destroy the CDK application for the $GEM_NAME gem
    popd
    return 1
fi
popd
return 0
}

echo [cdk_installation] Install nvm $NVM_VERSION
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/$NVM_VERSION/install.sh | bash
export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"  # This loads nvm
[ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion"  # This loads nvm bash_completion
echo [cdk_installation] Install the current version of nodejs
nvm install node

echo [cdk_installation] Install the latest version of CDK
if ! npm uninstall -g aws-cdk;
then
    echo [cdk_bootstrap] Failed to uninstall the current version of CDK
    exit 1
fi
if ! npm install -g aws-cdk@latest;
then
    echo [cdk_bootstrap] Failed to install the latest version of CDK
    exit 1
fi

# Set temporary AWS credentials from the assume role
credentials=$(aws sts assume-role --query Credentials.[SecretAccessKey,SessionToken,AccessKeyId] --output text --role-arn $ASSUME_ROLE_ARN --role-session-name o3de-Automation-session)
export AWS_SECRET_ACCESS_KEY=$(echo $credentials | cut -d' ' -f1)
export AWS_SESSION_TOKEN=$(echo $credentials | cut -d' ' -f2)
export AWS_ACCESS_KEY_ID=$(echo $credentials | cut -d' ' -f3)

if [[ -z "$O3DE_AWS_PROJECT_NAME" ]]; then
   export O3DE_AWS_PROJECT_NAME=$BRANCH_NAME-$PIPELINE_NAME-Linux
fi

ERROR_EXISTS=0
DestroyCDKApplication AWSCore
ERROR_EXISTS=$?
DestroyCDKApplication AWSClientAuth
ERROR_EXISTS=$?
DestroyCDKApplication AWSMetrics
ERROR_EXISTS=$?

if [ $ERROR_EXISTS -eq 1 ]
then
    exit 1
fi

exit 0

