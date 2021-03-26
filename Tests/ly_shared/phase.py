"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This represents one "phase" of a test.  The test runner communicates with the launcher by running phases and waiting
for results, however they appear.
"""

import logging
import os
import time
import xml.etree.ElementTree

import ly_test_tools.launchers.exceptions as exceptions
from ly_test_tools.launchers.platforms.base import Launcher

_POLL_INTERVAL_SEC = 1
_CRASH_TIMEOUT = 5

logger = logging.getLogger(__name__)


class Phase(object):
    """
    A generic test phase for running the launcher.  With the launcher running elsewhere (at a minimum in a different
    process, possibly on a different device altogether) the following kind of phases might occur:

    - Wait for a specific file to show up / complete processing.
    - Send commands to the launcher over the network and wait for response.
    - Wait for multiple launchers to coordinate networking testing.
    - Wait for a specific amount of time.

    Each phase can then compile the artifacts for the next phase.
    """
    def __init__(self, timeout):
        """
        :param timeout: Maximum time allocated for phase.
        """
        self.timeout = timeout

    def _start(self, previous_phase=None):
        """
        Start the phase.

        :return: None
        """
        logger.debug("start: {}".format(self.__class__.__name__))

    def _is_complete(self):
        """
        Check if the phase is complete.  This is the only required function.

        :return: None
        """
        raise NotImplementedError

    def _compile_artifacts(self):
        """
        Compile artifacts after a completed phase.
        """
        logger.debug("compile_artifacts: {}".format(self.__class__.__name__))

    def _update(self, elapsed_time):
        """
        Update the test phase if necessary.

        :param elapsed_time: Time since the last update.
        :return: None
        """
        logger.debug("update: {}".format(self.__class__.__name__))

    def _wait(self, launcher):
        """
        Wait for the phase to complete.

        :return: None.
        :raises: TimeoutError, CrashError
        """
        dead_time = -1
        logger.debug("wait begin: {}".format(self.__class__.__name__))
        start = time.time()

        while not self._is_complete():

            if time.time() - start > self.timeout:
                message = "Timeout exceeded {}s in {}".format(self.timeout, self.__class__.__name__)
                logger.error(message)
                raise exceptions.TimeoutError(message)
            elif not launcher.is_alive():
                # The final result may arrive after the app closes.
                if dead_time == -1:
                    dead_time = time.time()
                if time.time() - dead_time > _CRASH_TIMEOUT:
                    message = "Unexpected termination in {} after {:0.2f}s".format(
                        self.__class__.__name__, time.time() - start)
                    logger.error(message)
                    raise exceptions.CrashError(message)

            sleep_start = time.time()
            time.sleep(_POLL_INTERVAL_SEC)

            self._update(time.time() - sleep_start)

        logger.debug("wait end: {}, duration: {:.2f}".format(self.__class__.__name__, time.time() - start))


class FileExistsPhase(Phase):
    """
    Test phase that completes when a specific file is created.
    """
    def __init__(self, path, timeout=60, non_empty=False):
        super(FileExistsPhase, self).__init__(timeout)
        self.path = path
        self.non_empty = non_empty

    def _is_complete(self):
        if self.path is not None and os.path.exists(self.path):
            if self.non_empty:
                return os.path.getsize(self.path) > 0
            else:
                return True

        return False


class XMLValidPhase(FileExistsPhase):
    """
    Test phase that completes when a valid XML file is found.
    """
    def __init__(self, path, timeout=60):
        super(XMLValidPhase, self).__init__(path, timeout, non_empty=True)
        self.path = path
        self.xml = None

    def _is_complete(self):
        if not super(XMLValidPhase, self)._is_complete():
            return False

        try:
            self.xml = xml.etree.ElementTree.parse(self.path)
        except xml.etree.ElementTree.ParseError:
            return False

        return True


class TimePhase(Phase):
    """
    Simple class to complete in a specified time.  Can be used to test timeout.
    """
    def __init__(self, timeout, complete_time):
        super(TimePhase, self).__init__(timeout)
        self.complete_time = complete_time

    def _start(self, previous_phase=None):
        super(TimePhase, self)._start(previous_phase)
        self.start_time = time.time()

    def _is_complete(self):
        return time.time() - self.start_time > self.complete_time


class ElapsedTimePhase(Phase):
    """
    Simple class to complete in a specified time using elapsed time.  Can be used to test timeout and elapsed time.
    """
    def __init__(self, timeout, complete_time):
        super(ElapsedTimePhase, self).__init__(timeout)
        self.complete_time = complete_time
        self.total_time = None

    def _start(self, previous_phase=None):
        super(ElapsedTimePhase, self)._start(previous_phase)
        self.total_time = 0

    def _update(self, elapsed_time):
        super(ElapsedTimePhase, self)._update(elapsed_time)
        self.total_time += elapsed_time

    def _is_complete(self):
        return self.total_time > self.complete_time


class WaitForLauncherToQuit(Phase):
    def __init__(self, launcher, timeout=60):
        # type: (Launcher, int) -> None
        super(WaitForLauncherToQuit, self).__init__(timeout)
        self.launcher = launcher

    def _is_complete(self):
        # type: () -> bool
        return not self.launcher.is_alive()
