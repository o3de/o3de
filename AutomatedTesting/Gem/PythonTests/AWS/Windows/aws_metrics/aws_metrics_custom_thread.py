"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from threading import Thread


class AWSMetricsThread(Thread):
    """
    Custom thread for raising assertion errors on the main thread.
    """
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._error = None

    def run(self) -> None:
        try:
            super().run()
        except AssertionError as e:
            self._error = e

    def join(self, **kwargs) -> None:
        super().join(**kwargs)

        if self._error:
            raise AssertionError(self._error)
