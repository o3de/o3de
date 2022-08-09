# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
import config
import logging
from dynaconf import settings

# Logging
_MODULENAME = 'Tools.Python.MockTool.main'
_LOGGER = logging.getLogger(_MODULENAME)


# To get values from single environment (and default). This is to loop through all values, but to
# target individual values just use the "from_env(<target_environment>)"
for key, value in settings.items():
    if key in settings.from_env('maya'):
        _LOGGER.info(f"Key: {key}  Value: {value}")


# Check if a value is present- if it isn't will return the second value (in this case "404")
_LOGGER.info(f"Accessing Dynaconf Base Directory Variable: {settings.get('NO_KEY_AVAILABLE', 404)}")



