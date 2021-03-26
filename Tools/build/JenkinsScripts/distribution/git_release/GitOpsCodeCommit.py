############################################################################################
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
# a third party where indicated.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#############################################################################################

"""
This file is the central location for functions and operations for CodeCommit that are shared accross different scripts.
"""
import subprocess
import os
import git

# Initializes a Git repository configured with necessary settings to access CodeCommit via AWS CLI credentials.
def init_git_repo(aws_repo_url, awscli_profile, local_repo_directory):
    """
    Initializes a Git repository configured with necessary settings to access CodeCommit via AWS CLI credentials.

    :param aws_repo_url: Clone URL for the CodeCommit repo
    :param awscli_profile: AWS CLI profile with access/permissions to the repo.
    :param local_repo_directory: Clone directory
    :return:
    """

    parse_key = "amazonaws.com"
    host_domain_end_index = aws_repo_url.index(parse_key) + len(parse_key)
    host_domain = aws_repo_url[:host_domain_end_index]

    subprocess.call(["git", "init", local_repo_directory])

    config_append = f"""
[credential "{host_domain}"]
\thelper = !aws --profile {awscli_profile} codecommit credential-helper $@
\tUseHttpPath = true

[remote "origin"]
\turl = {aws_repo_url}
\tfetch = +refs/heads/*:refs/remotes/origin/*

[branch "master"]
\tremote = origin
\tmerge = refs/heads/master
"""

    config_filepath = os.path.join(local_repo_directory, '.git', 'config')

    with open(config_filepath, "a") as myfile:
        myfile.write(config_append)


def custom_clone(aws_repo_url, awscli_profile, local_repo_directory, setup_tracking_branches):
    print("Initializing local repo with custom AWS CodeCommit config...")
    initial_directory = os.getcwd()
    os.chdir(local_repo_directory)
    init_git_repo(aws_repo_url, awscli_profile, local_repo_directory)
    os.chdir(initial_directory)

    repo = git.Repo(local_repo_directory)

    print(f"Fetching from remote: {aws_repo_url}")
    repo.remote().fetch()  # Fetch branches
    repo.remote().fetch("--tags")  # Fetch tags

    if setup_tracking_branches:
        for remote_branch in repo.remote().refs:
            branch_name = remote_branch.remote_head
            repo.create_head(branch_name, remote_branch) \
                .set_tracking_branch(remote_branch)

    return repo
