# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------

import os

# -- envar util ----------------------------------------------------------
# not putting this in the env_util.py to reduce cyclical importing
def env_bool(envar, default=False):
    """! cast a env bool to a python bool
    :@param envar: str
        the envar key
    :@param default: bool
        the default value if not set"""
    envar_test = os.getenv(envar, default)
    # check 'False', 'false', and '0' since all are non-empty
    # env comes back as string and normally coerced to True.
    if envar_test in ('True', 'true', '1'):
        return True
    elif envar_test in ('False', 'false', '0'):
        return False
    else:
        return envar_test
# -------------------------------------------------------------------------
