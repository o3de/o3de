# coding:utf-8
#!/usr/bin/python
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

"""Boostraps and Starts Standalone DCC Material Converter utility"""

# built in's
import os
import site

# -------------------------------------------------------------------------
# \dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\SDK\PythonTools\DCC_Material_Converter\standalone.py
_MODULE_PATH = os.path.abspath(__file__)

_DCCSIG_REL_PATH = "../../../.."
_DCCSIG_PATH = os.path.join(_MODULE_PATH, _DCCSIG_REL_PATH)
_DCCSIG_PATH = os.path.normpath(_DCCSIG_PATH)

_DCCSIG_PATH = os.getenv('DCCSIG_PATH',
                         os.path.abspath(_DCCSIG_PATH))

# we don't have access yet to the DCCsi Lib\site-packages
site.addsitedir(_DCCSIG_PATH)  # PYTHONPATH

# azpy bootstrapping and extensions
import azpy.config_utils
_config = azpy.config_utils.get_dccsi_config()
settings = _config.get_config_settings(setup_ly_pyside=True)

from main import launch_material_converter

launch_material_converter()
