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
__copyright__ = "Copyright 2021, Amazon"
# -------------------------------------------------------------------------
import sys
import logging as _logging
# -------------------------------------------------------------------------

#pulling from azpy.constants causes cyclical imports :(
FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
_DCCSI_GDEBUG = False
_DCCSI_LOGLEVEL = int(20)
if _DCCSI_GDEBUG:
    _DCCSI_LOGLEVEL = int(10)
    
_PACKAGENAME = 'azpy.core'
_logging.basicConfig(format=FRMT_LOG_LONG, level=_DCCSI_LOGLEVEL)
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = []

if sys.version_info >= (3, 6):
    from azpy.core.py3.utils import get_datadir
elif sys.version_info >= (2, 6) and sys.version_info < (3, 6):
    _LOGGER.warning('Python vesion < 3 will be deprecated in the future')
    from azpy.core.py2.utils import get_datadir
else:
    _LOGGER.warning('Python vesion < 2.6 not recommended')
    from azpy.core.py2.utils import get_datadir

__all__ = ['get_datadir'] # you should hope it works

# -------------------------------------------------------------------------