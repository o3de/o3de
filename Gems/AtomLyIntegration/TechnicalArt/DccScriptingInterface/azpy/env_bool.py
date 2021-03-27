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
#
# -- This line is 75 characters -------------------------------------------

import os

# -- envar util ----------------------------------------------------------
# not putting this in the env_util.py to reduce cyclical importing
def env_bool(envar, default=False):
    """cast a env bool to a python bool"""
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