# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
Boostraps O3DE editor access to the python scripts within Atom.Feature.Commmon
Example: color grading related scripts
"""
# ------------------------------------------------------------------------
# standard imports
import os
import inspect
import site
from pathlib import Path
# ------------------------------------------------------------------------
_MODULENAME = 'Gems.Atom.Feature.Common.bootstrap'

# script directory
_MODULE_PATH = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
_MODULE_PATH = Path(_MODULE_PATH)
site.addsitedir(_MODULE_PATH.resolve())

from ColorGrading import initialize_logger
_LOGGER = initialize_logger(_MODULENAME, log_to_file=False)
_LOGGER.info(f'site.addsitedir({_MODULE_PATH.resolve()})')
# ------------------------------------------------------------------------