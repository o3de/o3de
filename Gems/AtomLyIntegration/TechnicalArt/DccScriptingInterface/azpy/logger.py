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
"""O3DE DCCsi Logger class. Convenience to reduce boilerplate logging setup"""
# -------------------------------------------------------------------------
# standard imports
import sys
import os
from pathlib import Path
import logging as _logging
from azpy.constants import STR_CROSSBAR
# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'azpy.logger'

_MODULE_PATH = Path(__file__)  # To Do: what if frozen?

_LOGGER = _logging.getLogger(_MODULENAME)

_LOGGER.debug(STR_CROSSBAR)
_LOGGER.debug('Initializing: {}'.format(_MODULENAME))
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH.as_posix()))
_LOGGER.info('This stub is an api placeholder: {}'.format(_MODULENAME))
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""
    import sys
    # return
    sys.exit()

