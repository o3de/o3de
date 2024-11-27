#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

class KeystoreSettings:
    """
    This class contains all the configuration parameters that are required
    to create a keystore for android applications.
    """
    DEFAULT_PASSWORD = "password"
    DEFAULT_KEY_ALIAS = "app_key_alias"
    DEFAULT_KEY_SIZE = "2048"
    DEFAULT_VALIDITY_DAYS = "10000"
    DEFAULT_CN = "MyAppName"
    DEFAULT_OU = "MyOrganizationalUnit"
    DEFAULT_O = "MyCompany"
    DEFAULT_C = "US"

    def __init__(self):
        self.keystore_file = ""
        self.keystore_password = self.DEFAULT_PASSWORD
        self.key_alias = self.DEFAULT_KEY_ALIAS
        self.key_password = (
            self.keystore_password
        )  # Must be same as store password.
        self.key_size = self.DEFAULT_KEY_SIZE
        self.validity_days = self.DEFAULT_VALIDITY_DAYS
        # Parts of a distiguished name string:
        self.dn_common_name = self.DEFAULT_CN
        self.dn_organizational_unit = self.DEFAULT_OU
        self.dn_organization = self.DEFAULT_O
        self.dn_country_code = self.DEFAULT_C


    def get_distinguished_name(self) -> str:
        return f"cn={self.dn_common_name}, ou={self.dn_organizational_unit}, o={self.dn_organization}, c={self.dn_country_code}"


    def configure_distinguished_name(self,
        app_name: str, organizational_unit: str,
        company_name: str, country_code: str):
        self.dn_common_name = app_name
        self.dn_organizational_unit = organizational_unit
        self.dn_organization = company_name
        self.dn_country_code = country_code


    @classmethod
    def from_dictionary(cls, d: dict) -> "KeystoreSettings":
        ks = KeystoreSettings()
        ks.keystore_file = d.get("keystore_file", "")
        ks.keystore_password = d.get("keystore_password", cls.DEFAULT_PASSWORD)
        ks.key_alias =  d.get("key_alias", cls.DEFAULT_KEY_ALIAS)
        ks.key_password = d.get("key_password", cls.DEFAULT_PASSWORD)
        ks.key_size = d.get("key_size", cls.DEFAULT_KEY_SIZE)
        ks.validity_days = d.get("validity_days", cls.DEFAULT_VALIDITY_DAYS)
        # Parts of a distiguished name string:
        ks.dn_common_name = d.get("dn_common_name", cls.DEFAULT_CN)
        ks.dn_organizational_unit = d.get("dn_organizational_unit", cls.DEFAULT_OU)
        ks.dn_organization = d.get("dn_organization", cls.DEFAULT_O)
        ks.dn_country_code = d.get("dn_country_code", cls.DEFAULT_C)
        return ks


    def as_dictionary(self) -> dict:
        d = {
            "keystore_file" : self.keystore_file,
            "keystore_password" : self.keystore_password,
            "key_alias" : self.key_alias,
            "key_password" : self.key_password,
            "validity_days" : self.validity_days,
            "dn_common_name" : self.dn_common_name,
            "dn_organizational_unit" : self.dn_organizational_unit,
            "dn_organization" : self.dn_organization,
            "dn_country_code" : self.dn_country_code,
        }
        return d

# class KeystoreSettings END
######################################################
