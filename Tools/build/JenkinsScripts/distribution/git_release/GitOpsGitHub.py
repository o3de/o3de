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
This file is the central location for functions and operations for GitHub that are shared accross different scripts.
"""
from urllib.parse import quote_plus
from github import Github, BadCredentialsException


def create_authenticated_https_clone_url(github_user, github_password, https_endpoint_url):
        # Windows does not ship with SSH, and we need a universally usable approach.
        # This is why we opt for HTTPS authentication. Without user input, we will
        # need to inject username/password into the repo url at
        # 'auth_insertion_offset'.
        # Example resulting URL- https://username:password@endpoint.com/repo.git
        auth_insertion_offset = 8

        # If you have a symbol like @ in your username or password, it will mess up the url command to clone.
        # urllib.quote_plus replaces special characters with url safe characters, turning @ into %40.
        url_safe_password = quote_plus(github_password)
        url_safe_user = quote_plus(github_user)

        authenticated_url = "{0}{1}:{2}@{3}".format(https_endpoint_url[:auth_insertion_offset],
                                                    url_safe_user,
                                                    url_safe_password,
                                                    https_endpoint_url[auth_insertion_offset:])
        return authenticated_url


def are_credentials_valid(username, password):
    auth_test_obj = Github(username, password)

    try:
        for repo in auth_test_obj.get_user().get_repos():
            # Will raise 'Bad Credentials' exception if can't find name.
            repo.name
    except BadCredentialsException:
        return False

    return True
