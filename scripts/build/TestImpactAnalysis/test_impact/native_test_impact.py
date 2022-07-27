#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from test_impact import BaseTestImpact
from tiaf_logger import get_logger

logger = get_logger(__file__)


class NativeTestImpact(BaseTestImpact):

    _runtime_type = "native"
    _default_sequence_type = "regular"

    def _parse_arguments_to_runtime(self, args, sequence_type, runtime_args):
        super()._parse_arguments_to_runtime(
            args, sequence_type, runtime_args)

        if args.get('safe_mode'):
            runtime_args.append("--safemode=on")
            logger.info("Safe mode set to 'on'.")
        else:
            runtime_args.append("--safemode=off")
            logger.info("Safe mode set to 'off'.")

        return runtime_args

    @property
    def default_sequence_type(self):
        return self._default_sequence_type

    @property
    def runtime_type(self):
        return self._runtime_type
