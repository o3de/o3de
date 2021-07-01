#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import boto3
import logging
import os
import subprocess
import sys

from botocore.exceptions import ClientError
from urllib.parse import urlparse, urlunparse

log = logging.getLogger(__name__)
log.setLevel(logging.INFO)

DEFAULT_BRANCH = "main"
DEFAULT_WORKSPACE_ROOT = "."


class MergeError(Exception):
    pass


class SyncRepo:
    """A git repo with configured remotes to sync with GitHub.

    Used by the sync pipeline to push branches to GitHub and pull down latest from main. Changes flow from
    the upstream remote down to origin. Remotes can be swapped to pull changes in the other direction.

    Attributes:
        origin: URL for the origin repo. This is the target for the sync.
        upstream: URL for the upstream repo. This is the source with the latest changes.
        workspace_root: Path to the parent directory for the local workspace.
        parameter: Name of the parameter used to store GitHub credentials.

    """

    def __init__(self, origin, upstream, workspace_root, region=None, parameter=None):
        self.workspace_root = workspace_root
        self.parameter = parameter
        self.region = region

        if self.parameter and self.region:
            log.info(f"Adding credentials from {self.parameter} in {self.region}")
            self.origin = self._add_credentials(origin)
            self.upstream = self._add_credentials(upstream)
        else:
            self.origin = origin
            self.upstream = upstream

        self.origin_name = self.origin.split("/")[-1]
        self.upstream_name = self.upstream.split("/")[-1]
        self.workspace = os.path.join(workspace_root, self.origin_name)

    def _add_credentials(self, url):
        """Add credentials to a github repo URL from parameter store."""
        parsed_url = urlparse(url)
        if parsed_url.netloc == "github.com":
            try:
                ssm = boto3.client("ssm", self.region)
                credentials = ssm.get_parameter(
                    Name=self.parameter,
                    WithDecryption=True
                )["Parameter"]["Value"]
                url = urlunparse(parsed_url._replace(netloc=f"{credentials}@github.com"))
            except ClientError as e:
                log.error(f"Error retrieving credentials from parameter store: {e}")
        return url

    def clone(self):
        """Clones repo to the instance workspace. Refreshes remote configs for existing repos."""
        if not os.path.exists(self.workspace):
            os.mkdir(self.workspace)

        if subprocess.run(["git", "rev-parse", "--is-inside-work-tree"], cwd=self.workspace).returncode != 0:
            log.info(f"Cloning repo {self.origin} to {self.workspace}.")
            subprocess.run(["git", "clone", self.origin, self.origin_name], cwd=self.workspace_root, check=True)
            subprocess.run(["git", "remote", "add", "upstream", self.upstream], cwd=self.workspace)
        else:
            log.info("Update remote config for existing repos.")
            subprocess.run(["git", "remote", "set-url", "origin", self.origin], cwd=self.workspace)
            subprocess.run(["git", "remote", "set-url", "upstream", self.upstream], cwd=self.workspace)

    def sync(self, branch):
        """Fetches latest from upstream and syncs changes to origin.

        Syncs are one-way and conflicts are not expected. Fast-forward merges are performed if possible. If a
        fast-forward merge is not possible, a merge will not be attempted and will raise an exception.

        The checkout command will create a new branch from upstream/<branch> if it does not exist in origin. The
        remote will be remapped to origin during the push.

        Args:
            branch: Name of the upstream branch to sync with origin.

        Raises:
            MergeError: An error occured when attempting to merge to the target branch.

        """
        subprocess.run(["git", "fetch", "origin"], cwd=self.workspace, check=True)
        subprocess.run(["git", "fetch", "upstream"], cwd=self.workspace, check=True)
        subprocess.run(["git", "checkout", branch], cwd=self.workspace, check=True)

        # If the branch exists in origin, merge from upstream. New branches do not require a merge.
        if subprocess.run(["git", "ls-remote", "--exit-code", "-h", "origin", branch], cwd=self.workspace).returncode == 0:
            subprocess.run(["git", "reset", "--hard", "HEAD"], cwd=self.workspace, check=True)
            subprocess.run(["git", "pull"], cwd=self.workspace, check=True)

            if subprocess.run(["git", "merge", "--ff-only", f"upstream/{branch}"], cwd=self.workspace).returncode != 0:
                raise MergeError(f"Unable to perform ff merge to target branch: {self.origin}/{branch} Intervention required.")

        subprocess.run(["git", "push", "-u", "origin", branch], cwd=self.workspace, check=True)


def process_args():
    """Process arguements.

    Example:
        sync_repo.py <upstream> <origin> [Options]

    """
    parser = argparse.ArgumentParser()
    parser.add_argument("upstream")
    parser.add_argument("origin")
    parser.add_argument("-b", "--branch", default=DEFAULT_BRANCH)
    parser.add_argument("-w", "--workspace-root", default=DEFAULT_WORKSPACE_ROOT)
    parser.add_argument("-r", "--region", default=None)
    parser.add_argument("-p", "--parameter", default=None)
    return parser.parse_args()


def main():
    args = process_args()

    repo = SyncRepo(args.origin, args.upstream, args.workspace_root, args.region, args.parameter)
    repo.clone()
    try:
        repo.sync(args.branch)
    except MergeError as e:
        log.error(e)


if __name__ == "__main__":
    sys.exit(main())
