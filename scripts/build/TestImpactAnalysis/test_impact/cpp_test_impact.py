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


class CPPTestImpact(BaseTestImpact):

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

    def _set_default_sequence_type(self):
        return "regular"

    def _generate_result(self, s3_bucket: str, suite: str, return_code: int, report: dict, runtime_args: list):
        result = super()._generate_result(
            s3_bucket, suite, return_code, report, runtime_args)

        return result
