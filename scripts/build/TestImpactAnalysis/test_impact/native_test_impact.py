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

ARG_SAFE_MODE = "safe_mode"


class NativeTestImpact(BaseTestImpact):

    _runtime_type = "native"
    _default_sequence_type = "regular"

    def _parse_arguments_to_runtime(self, args, sequence_type, runtime_args):
        """
        Fetches the relevant keys from the provided dictionary, and applies the values of the arguments(or applies them as a flag) to our runtime_args list.

        @param args: Dictionary containing the arguments passed to this TestImpact object. Will contain all the runtime arguments we need to apply.
        @sequence_type: The sequence type as determined when initialising this TestImpact object.
        @runtime_args: A list of strings that will become the arguments for our runtime.
        """
        super()._parse_arguments_to_runtime(
            args, sequence_type, runtime_args)

        if args.get(ARG_SAFE_MODE):
            runtime_args.append("--safemode=on")
            logger.info("Safe mode set to 'on'.")
        else:
            runtime_args.append("--safemode=off")
            logger.info("Safe mode set to 'off'.")

        return runtime_args

    @property
    def default_sequence_type(self):
        """
        The default sequence type for this TestImpact class. Must be implemented by subclass.
        """
        return self._default_sequence_type

    @property
    def runtime_type(self):
        """
        The runtime this TestImpact supports. Must be implemented by subclass
        Current options are "native" or "python"
        """
        return self._runtime_type
