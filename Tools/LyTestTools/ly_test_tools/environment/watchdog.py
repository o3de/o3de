"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

The Watchdog parent class that spawns a separate thread to wait for a specific condition. If that condition is
fulfilled, then it can raise an exception or log an error.
"""
import threading
import logging
import os
import re
import psutil
import time

import ly_test_tools
import ly_test_tools.environment.process_utils as process_utils

logger = logging.getLogger(__name__)


class WatchdogError(Exception):
    """ Indicates that a Watchdog met its exception condition """


class Watchdog(object):
    DEFAULT_JOIN_TIMEOUT = 10  # seconds

    def __init__(self, bool_fn, interval=1, raise_on_condition=True, name='ly_test_watchdog', error_message=''):
        # type: (function, int, bool, str, str) -> Watchdog
        """
        A Watchdog object that takes in a boolean function. It spawns a thread that loops over the boolean function
        until it returns True. If the boolean function returns True, then a flag will be set and an exception
        will be raised when stop() is called.

        :param bool_fn: A function that must return a boolean. It will be ran by the spawned thread until it's True
        :param interval: The interval (in seconds) for how frequently the bool_fn is called on the thread.
        :param raise_on_condition: If True, raises an exception when bool_fn returns True. If False, logs an error message
        when bool_fn returns True.
        :param name: The name of the thread.
        :param error_message: The error message to log when bool_fn returns True. Defaults to printing the watchdog name
        and function name.
        """
        self.caught_failure = False
        self.name = name

        self._bool_fn = bool_fn
        self._interval = interval
        self._raise_on_condition = raise_on_condition
        default_error_message = f'Watchdog: {name} caught an unexpected condition from function: {bool_fn.__name__}()'
        self._error_message = error_message if error_message else default_error_message
        self._shutdown = threading.Event()
        self._watchdog_thread = threading.Thread(target=self._watchdog, name=name)

    def start(self):
        # type: () -> None
        """
        Starts the watchdog's thread which spawns it and begins executing its target function. Also clears the thread's
        shutdown Event. Clears the caught_failure attribute if the thread has been restarted.

        :return: None
        """
        self._shutdown.clear()
        self._watchdog_thread.start()
        self.caught_failure = False

    def stop(self, join_timeout=DEFAULT_JOIN_TIMEOUT):
        # type: (int) -> None
        """
        Stops the watchdog's thread if it's executing by enabling its shutdown Event. If the target function's condition
        was found, then it will either raise an exception or log an error message.

        :param join_timeout: The timeout to wait for the watchdog thread to join.
        :return: None
        """
        # Set the Event attribute so that the thread stops running
        self._shutdown.set()

        # Join the watchdog thread back to primary thread
        self._watchdog_thread.join(timeout=join_timeout)
        if self.is_alive():
            # thread has timed out because it's still alive
            logger.error(f'Thread: {self.name} timed out when calling join()')

        # No further action taken if nothing was caught
        if not self.caught_failure:
            return

        # either raise an exception or log an error
        if self._raise_on_condition:
            raise WatchdogError(self._error_message)
        logger.error(self._error_message)

    def is_alive(self):
        # type: () -> bool
        """
        The thread is killed when it times out or its target returns True. Timed out threads are considered not alive.

        :return: Returns True if the thread is alive, else False
        """
        return self._watchdog_thread.is_alive()

    def _watchdog(self):
        # type: () -> None
        """
        The main function of the watchdog thread. It will repeatedly call its target function until the target function
        returns True, in which it will set the self.caught_failure attribute to True.

        :return: None
        """
        while True:
            # check to see if the thread is shut down
            if self._shutdown.wait(timeout=self._interval):
                logger.info(f"Shutting down watchdog: {self.name}")
                return
            # call the target function and see if it returned True
            if self._bool_fn():
                self.caught_failure = True
                return


class ProcessUnresponsiveWatchdog(Watchdog):
    def __init__(self, process_id, interval=1, raise_on_condition=True, name='process_watchdog', error_message='',
                 unresponsive_timeout_seconds=30):
        # type: (int, int, bool, str, str, int) -> ProcessUnresponsiveWatchdog
        """
        Watches a process ID and reports if it is unresponsive for a given timeout. If multiple processes need to be
        watched, then multiple watchdogs should be instantiated.
        Note: This is for windows OS only.

        :param process_id: The process id to watch
        :param interval: The interval (in seconds) for how frequently the bool_fn is called on the thread.
        :param raise_on_condition: If True, raises an exception when bool_fn returns True. If False, logs an error
        message when bool_fn returns True.
        :param name: The name of the thread.
        :param error_message: The error message when bool_fn returns True. Defaults to the watchdog name and pid
        :param unresponsive_timeout_seconds: How long the process needs to be unresponsive for in order for the watchdog
        to report (in seconds).
        """
        if not ly_test_tools.WINDOWS:
            pass # TODO add non-windows support
        self._unresponsive_timeout = unresponsive_timeout_seconds
        self._calculated_timeout_point = None
        self._pid = process_id
        self._process_name = psutil.Process(self._pid).name()
        self._default_error_message = f"Process watchdog has found that process: {self._process_name} with pid:  " \
                                      f"{self._pid} was unresponsive during the test run. Investigate process " \
                                      f"{self._process_name} for failures."
        if not error_message:
            error_message = self._default_error_message

        super(ProcessUnresponsiveWatchdog, self).__init__(bool_fn=self._process_not_responding, interval=interval,
                                                          raise_on_condition=raise_on_condition, name=name,
                                                          error_message=error_message)

    def _process_not_responding(self):
        # type: () -> bool
        """
        Checks to see if the process has been unresponsive longer than self._unresponsive_timeout. Once the process is
        found to be unresponsive, it will keep track of how long it has been unresponsive for.

        The cmd returns a string of not responding tasks in the following format:

        "b'\r\n
        Image Name                     PID Session Name        Session#    Mem Usage\r\n
        ========================= ======== ================ =========== ============\r\n
        foo.exe                      00000 Console                    1          0 K\r\n'"

        :return: True if the process has been unresponsive for self._unresponsive_timeout seconds, else False
        """
        _PROCESS_OUTPUT_ENCODING = 'utf-8'
        cmd = 'tasklist /FI "PID eq %d" /FI "STATUS eq not responding"' % self._pid
        # searches for a process name and pid with white space in between them and at the end
        regex = f'{self._process_name}\s+{self._pid}\s'
        status = process_utils.check_output(cmd)
        if re.search(regex, status):
            if self._calculated_timeout_point is None:
                # First instance of process unresponsive
                self._calculated_timeout_point = time.time() + self._unresponsive_timeout
            elif time.time() >= self._calculated_timeout_point:
                return True
        else:
            # Process is responsive
            self._calculated_timeout_point = None
        return False

    def get_pid(self):
        # type: () -> int
        """
        Get the pid that the watchdog is watching.
        
        :return: The process id of the watchdog
        """
        return self._pid


class CrashLogWatchdog(Watchdog):
    def __init__(self, log_path, interval=1, raise_on_condition=True, name='crash_log_watchdog', error_message=''):
        # type: (str, int, bool, str, str) -> CrashLogWatchdog
        """
        A watchdog that watches if a file gets created and reports if it finds the file. The watchdog will check to see
        if the file already exists and removes it before starting to watch.

        :param log_path: The absolute path of the log to watch
        :param interval: The interval (in seconds) for how frequently the bool_fn is called on the thread.
        :param raise_on_condition: If True, raises an exception when bool_fn returns True. If False, logs an error message
        when bool_fn returns True.
        :param name: The name of the thread.
        :param error_message: The error message to log when bool_fn returns True. Defaults to printing the watchdog name
        """
        self._log_path = log_path

        def crash_exists():
            return os.path.exists(log_path)

        if not error_message:
            error_message = f"Crash log watchdog has detected an error log found at {log_path} during the test run. \
                            Investigate the process that creates the error log for failures."

        if os.path.exists(log_path):
            logger.info(f"Removing existing {log_path} when initializing crash log watchdog.")
            os.remove(log_path)

        super(CrashLogWatchdog, self).__init__(bool_fn=crash_exists, interval=interval,
                                               raise_on_condition=raise_on_condition,
                                               name=name, error_message=error_message)

    def stop(self):
        # type: () -> None
        """
        Prints the crash log if able when stopping the watchdog.

        :return: None
        """
        header = "================= Crash Log Print =================\n"
        if self.caught_failure:
            print(header)
            with open(self._log_path, "r") as crash_log:
                for line in crash_log:
                    print(line.strip('\n'))
            print("=" * len(header) + "\n")

        super(CrashLogWatchdog, self).stop()