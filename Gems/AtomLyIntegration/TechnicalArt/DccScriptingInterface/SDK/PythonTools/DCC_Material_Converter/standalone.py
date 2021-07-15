# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

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
