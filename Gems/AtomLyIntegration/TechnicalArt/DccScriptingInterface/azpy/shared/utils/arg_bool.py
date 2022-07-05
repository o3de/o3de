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
"""! @brief
Module Documentation:
<DCCsi>/azpy/shared/utils/arg_bool.py

Contains a util for working with bools in argparse cli modules.
"""

# -------------------------------------------------------------------------
def arg_bool(bool_arg, desc='arg desc not set'):
    """cast a arg bool to a python bool"""

    _LOGGER.info(f"Checking '{desc}': {bool_arg}")

    if bool_arg in ('True', 'true', '1'):
        return True
    elif bool_arg in ('False', 'false', '0'):
        return False
    else:
        return bool_arg
# -------------------------------------------------------------------------
