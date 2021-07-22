"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import boto3
import configparser
import logging
import os
import pytest
import typing

logger = logging.getLogger(__name__)
logging.getLogger('boto').setLevel(logging.CRITICAL)


class AwsCredentials:
    def __init__(self, profile_name: str):
        self._profile_name = profile_name

        self._credentials_path = os.environ.get('AWS_SHARED_CREDENTIALS_FILE')
        if not self._credentials_path:
            # Home directory location varies based on the operating system, but is referred to using the environment
            # variables %UserProfile% in Windows and $HOME or ~ (tilde) in Unix-based systems.
            self._credentials_path = os.path.join(os.environ.get('UserProfile', os.path.expanduser('~')),
                                                  '.aws', 'credentials')
        self._credentials_file_exists = os.path.exists(self._credentials_path)

        self._credentials = configparser.ConfigParser()
        self._credentials.read(self._credentials_path)

    def get_aws_credentials(self) -> typing.Tuple[str, str, str]:
        """
        Get aws credentials stored in the specific named profile.

        :return AWS credentials.
        """
        access_key_id = self._get_aws_credential_attribute_value('aws_access_key_id')
        secret_access_key = self._get_aws_credential_attribute_value('aws_secret_access_key')
        session_token = self._get_aws_credential_attribute_value('aws_session_token')

        return access_key_id, secret_access_key, session_token

    def set_aws_credentials_by_session(self, session: boto3.Session) -> None:
        """
        Set AWS credentials stored in the specific named profile using an assumed role session.

        :param session: assumed role session.
        """
        credentials = session.get_credentials().get_frozen_credentials()
        self.set_aws_credentials(credentials.access_key, credentials.secret_key, credentials.token)

    def set_aws_credentials(self, aws_access_key_id: str, aws_secret_access_key: str,
                            aws_session_token: str) -> None:
        """
        Set AWS credentials stored in the specific named profile.

        :param aws_access_key_id: AWS access key id.
        :param aws_secret_access_key: AWS secrete access key.
        :param aws_session_token: AWS assumed role session.
        """
        self._set_aws_credential_attribute_value('aws_access_key_id', aws_access_key_id)
        self._set_aws_credential_attribute_value('aws_secret_access_key', aws_secret_access_key)
        self._set_aws_credential_attribute_value('aws_session_token', aws_session_token)

        if (len(self._credentials.sections()) == 0) and (not self._credentials_file_exists):
            os.remove(self._credentials_path)
            return

        with open(self._credentials_path, 'w+') as credential_file:
            self._credentials.write(credential_file)

    def _get_aws_credential_attribute_value(self, attribute_name: str) -> str:
        """
        Get the value of an AWS credential attribute stored in the specific named profile.

        :param attribute_name: Name of the AWS credential attribute.
        :return Value of the AWS credential attribute.
        """
        try:
            value = self._credentials.get(self._profile_name, attribute_name)
        except configparser.NoSectionError:
            # Named profile or key doesn't exist
            value = None
        except configparser.NoOptionError:
            # Named profile doesn't have the specified attribute
            value = None

        return value

    def _set_aws_credential_attribute_value(self, attribute_name: str, attribute_value: str) -> None:
        """
        Set the value of an AWS credential attribute stored in the specific named profile.

        :param attribute_name: Name of the AWS credential attribute.
        :param attribute_value: Value of the AWS credential attribute.
        """
        if self._profile_name not in self._credentials:
            self._credentials[self._profile_name] = {}

        if attribute_value is None:
            self._credentials.remove_option(self._profile_name, attribute_name)
            # Remove the named profile if it doesn't have any AWS credential attribute.
            if len(self._credentials[self._profile_name]) == 0:
                self._credentials.remove_section(self._profile_name)
        else:
            self._credentials[self._profile_name][attribute_name] = attribute_value

