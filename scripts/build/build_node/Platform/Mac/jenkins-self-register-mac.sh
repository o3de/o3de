#!/bin/bash 
set -euo pipefail

# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set +x

# Function to get EC2 metadata within the instance
get_metadata () {
    local response=$(curl -GLsX GET http://169.254.169.254/latest/meta-data/$1)
    echo "${response}"
}

# Get instance data
INSTANCE_ID=$(get_metadata "instance-id")
REGION=$(get_metadata "placement/availability-zone" | sed 's/.$//')
INSTANCE_PRIVATE_IP=$(get_metadata "local-ipv4")
INSTANCE_PUBLIC_IP=$(get_metadata "public-ipv4")
INSTANCE_NAME=$(aws ec2 describe-tags --filters "Name=resource-id,Values=$INSTANCE_ID" "Name=key,Values='Name'" --region $REGION --output=text --query="Tags[0].Value" | tr -d '\r')

# Configuration parameters
CONTROLLER_URL=$1
CONTROLLER_SECRET_ID=$2
BUILD_USER=$(aws ssm get-parameters --names "shared.builderuser" --region $region --with-decryption | ConvertFrom-Json)
CONTROLLER_USERNAME=$(aws secretsmanager get-secret-value --secret-id "${CONTROLLER_SECRET_ID}" --query 'SecretString' --output text | python -c 'import sys, json;print(json.load(sys.stdin)["username"])')
CONTROLLER_PASSWORD=$(aws secretsmanager get-secret-value --secret-id "${CONTROLLER_SECRET_ID}" --query 'SecretString' --output text | python -c 'import sys, json;print(json.load(sys.stdin)["apitoken"])')
SSH_KEY="${BUILD_USER}-ssh-key"
NODE_NAME="EC2 (${REGION}) - ${INSTANCE_NAME} (${INSTANCE_ID})"
NODE_LABEL="mac-${REGION}"
ENV_PATH="$PATH:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin"


# Download CLI jar from the controller
curl "${CONTROLLER_URL}"/jnlpJars/jenkins-cli.jar -o ~/jenkins-cli.jar

# Create node according to parameters passed in
cat <<EOF | java -jar ~/jenkins-cli.jar -auth "${CONTROLLER_USERNAME}:${CONTROLLER_PASSWORD}" -s "${CONTROLLER_URL}" create-node "${NODE_NAME}" |true
<slave>
  <name>${NODE_NAME}</name>
  <description>Ext IP: ${INSTANCE_PUBLIC_IP}</description>
  <remoteFS>/Users/lybuilder/jenkins</remoteFS>
  <numExecutors>1</numExecutors>
  <mode>EXCLUSIVE</mode>
  <retentionStrategy class="hudson.slaves.RetentionStrategy$Always"/>
  <launcher class="hudson.plugins.sshslaves.SSHLauncher" plugin="ssh-slaves@1.31.5">
    <host>${INSTANCE_PRIVATE_IP}</host>
    <port>22</port>
    <credentialsId>${SSH_KEY}</credentialsId>
    <launchTimeoutSeconds>60</launchTimeoutSeconds>
    <maxNumRetries>10</maxNumRetries>
    <retryWaitTime>15</retryWaitTime>
    <sshHostKeyVerificationStrategy class="hudson.plugins.sshslaves.verifiers.NonVerifyingKeyVerificationStrategy"/>
    <tcpNoDelay>true</tcpNoDelay>
  </launcher>
  <label>${NODE_LABEL}</label>
  <nodeProperties>
    <hudson.slaves.EnvironmentVariablesNodeProperty>
      <envVars serialization="custom">
        <unserializable-parents/>
        <tree-map>
          <default>
            <comparator class="hudson.util.CaseInsensitiveComparator"/>
          </default>
          <int>1</int>
          <string>PATH</string>
          <string>${ENV_PATH}</string>
        </tree-map>
      </envVars>
    </hudson.slaves.EnvironmentVariablesNodeProperty>
  </nodeProperties>
</slave>
EOF