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


# --------------------------------------------------------------------------
import os
import sys
import logging as _logging
# --------------------------------------------------------------------------


# -------------------------------------------------------------------------
#  global space debug flag
# not using azpy.constants here to help avoid cyclical imports lower
from azpy.env_bool import env_bool

# have to avoid importing these from constants
# because we can end up with cyclical import issues
# need to figure out a better solution later so we don't duplicate everywhere
ENVAR_DCCSI_GDEBUG = str('DCCSI_GDEBUG')
ENVAR_DCCSI_DEV_MODE = str('DCCSI_DEV_MODE')

#  global space
_G_DEBUG = os.getenv(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = os.getenv(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = __name__
if _PACKAGENAME is '__main__':
    _PACKAGENAME = 'azpy.return_stub'

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['return_stub']
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def return_stub(stub):
    _dir_to_last_file = None
    '''Take a file name (stub) and returns the directory of the file (stub)'''
    if _dir_to_last_file is None:
        path = os.path.abspath(__file__)
        while 1:
            path, tail = os.path.split(path)
            if (os.path.isfile(os.path.join(path, stub))):
                break
            if (len(tail) == 0):
                path = ""
                if _G_DEBUG:
                    print('~ Debug Message:  I was not able to find the '
                          'path to that file (stub) in a walk-up from currnet path')
                break
        _dir_to_last_file = path

    return _dir_to_last_file
# --------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    # there are not really tests to run here due to this being a list of
    # constants for shared use.

    # happy print
    print("# {0} #".format('-' * 72))
    print('~ find_stub.py ... Running script as __main__')
    print("# {0} #\r".format('-' * 72))

    print('~   Current Work dir: {0}'.format(os.getcwd()))

    print('~   Dev\: {0}'.format(return_stub('engineroot.txt')))

    # custom prompt
    sys.ps1 = "[azpy]>>"
