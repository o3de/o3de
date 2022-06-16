"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Functions to aid in monitoring log files being actively written to for a set of lines to read for.
"""
import logging
import os
import re

import ly_test_tools.environment.waiter as waiter
import ly_test_tools.launchers.platforms.base

logger = logging.getLogger(__name__)

LOG_MONITOR_INTERVAL = 0.1  # seconds


class LogMonitorException(Exception):
    """Custom exception class for errors related to log_monitor.py"""

    pass


def check_exact_match(line, expected_line):
    """
    Uses regular expressions to find an exact (not partial) match for 'expected_line' in 'line', i.e.
    in the example below it matches 'foo' and succeeds:
        line value: '66118.999958 - INFO - [MainThread] - ly_test_tools.o3de.asset_processor - foo'
        expected_line: 'foo'

    :param line: The log line string to search,
        i.e. '9189.9998188 - INFO - [MainThread] - example.tests.test_system_example - Log Monitoring test 1'
    :param expected_line: The exact string to match when searching the line param,
        i.e. 'Log Monitoring test 1'
    :return: An exact match for the string if one is found, None otherwise.
    """

    # Look for either start of line or whitespace, then the expected_line, then either end of the line or whitespace.
    # This way we don't partial match inside of a string.  So for example, 'foo' matches 'foo bar' but not 'foobar'
    regex_pattern = re.compile("(^|\\s){}($|\\s)".format(re.escape(expected_line)), re.UNICODE)
    if regex_pattern.search(line) is not None:
        return expected_line

    return None

class LogMonitor(object):

    def __init__(self, launcher, log_file_path, log_creation_max_wait_time=5):
        """
        Log monitor object for monitoring a single log file for expected or unexpected line values.
        Requires a launcher class & valid log file path.

        :param launcher: Launcher class object that opens a locally-accessible log file to write to.
        :param log_file_path: string representing the path to the file to open.
        :param log_creation_max_wait_time: max time to wait in seconds for log to exist
        """
        self.unexpected_lines_found = []
        self.expected_lines_not_found = []
        self.launcher = launcher
        self.log_file_path = log_file_path
        self.py_log = ""
        self.log_creation_max_wait_time = log_creation_max_wait_time

    def monitor_log_for_lines(self,
                              expected_lines=None,
                              unexpected_lines=None,
                              halt_on_unexpected=False,
                              timeout=30):
        """
        Monitor for expected or unexpected lines for the log file attached to this LogMonitor object.
        Returns True on success or raises LogMonitorException on failure.
        Will search for X seconds where X is the value of the timeout parameter.

        :param expected_lines: list of strings that the user wants to find in the self.log_file_path file.
        :param unexpected_lines: list of strings that must not be present in the self.log_file_path file.
        :param halt_on_unexpected: boolean to determine whether to raise LogMonitorException on the first
            unexpected line found (True) or not (False)
        :param timeout: int time in seconds to search for expected/unexpected lines before raising LogMonitorException.
        :return: True if monitoring succeeded, raises a LogMonitorException otherwise.
        """
        waiter.wait_for(
            lambda: os.path.exists(self.log_file_path),
            timeout=self.log_creation_max_wait_time,
            exc=LogMonitorException(f"Log file '{self.log_file_path}' was never created by another process."),
            interval=1,
        )

        # Validation checks before monitoring the log file writes.
        launcher_class = ly_test_tools.launchers.platforms.base.Launcher
        if not os.path.exists(self.log_file_path):
            raise LogMonitorException(
                "Referenced self.log_file_path file does not exist: {}".format(self.log_file_path))
        if not isinstance(self.launcher, launcher_class):
            raise LogMonitorException(
                "Referenced launcher type: '{}' is not a valid launcher class. Must be of type: '{}'".format(
                    type(self.launcher), launcher_class))
        if not expected_lines and not unexpected_lines:
            logger.warning("Requested log monitoring for no lines, aborting")
            return

        # Enforce list typing for expected_lines & unexpected_lines
        if unexpected_lines is None:
            unexpected_lines = []
        if expected_lines is None:
            expected_lines = []
            logger.warning(
                "Requested log monitoring without providing any expected lines. "
                "Log monitoring will continue for '{}' seconds to search for unexpected lines.".format(timeout))
        if type(expected_lines) is not list or type(unexpected_lines) is not list:
            raise LogMonitorException(
                "expected_lines or unexpected_lines must be 'list' type variables. "
                "Got types: type(expected_lines) == {} & type(unexpected_lines) == {}".format(
                    type(expected_lines), type(unexpected_lines)))
                    
        # Make sure the expected_lines don't have any common lines with unexpected_lines
        expected_lines_in_unexpected = [line for line in unexpected_lines if line in expected_lines]
        if expected_lines_in_unexpected:
            raise LogMonitorException("Found unexpected_lines in expected_lines:\n{}".format("\n".join(expected_lines_in_unexpected)))
            
        unexpected_lines_in_expected = [line for line in expected_lines if line in unexpected_lines]
        if unexpected_lines_in_expected:
            raise LogMonitorException("Found expected_lines in unexpected_lines:\n{}".format("\n".join(unexpected_lines_in_expected)))

        # Log file is now opened by our process, start monitoring log lines:
        self.py_log = ""
        try:
            logger.debug("Monitoring log file in '{}' ".format(self.log_file_path))
            with open(self.log_file_path, mode='r', encoding='utf-8') as log:
                logger.info(
                    "Monitoring log file '{}' for '{}' seconds".format(self.log_file_path, timeout))
                    
                search_expected_lines = expected_lines.copy()
                search_unexpected_lines = unexpected_lines.copy()
                waiter.wait_for(  # Sets the values for self.unexpected_lines_found & self.expected_lines_not_found
                    lambda: self._find_lines(log, search_expected_lines, search_unexpected_lines, halt_on_unexpected),
                    timeout=timeout,
                    interval=LOG_MONITOR_INTERVAL)
        except AssertionError:  # Raised by waiter when timeout is reached.
            logger.warning(f"Timeout of '{timeout}' seconds was reached, log lines may not have been found")
            # exception will be raised below by _validate_results with failure analysis
        finally:
            logger.info("Python log output:\n" + self.py_log)
            logger.info(
                "Finished log monitoring for '{}' seconds, validating results.\n"
                "expected_lines_not_found: {}\n unexpected_lines_found: {}".format(
                    timeout, self.expected_lines_not_found, self.unexpected_lines_found))

        return self._validate_results(self.expected_lines_not_found, self.unexpected_lines_found, expected_lines, unexpected_lines)

    def _find_expected_lines(self, line, expected_lines):
        """
        Checks for any matches between the 'line' string and strings in the 'expected_lines' list.
        Removes expected_lines strings that are found from the main expected_lines list and returns the remaining
        expected_lines list values.

        :param line: string from a TextIO or BinaryIO file object being read line by line.
        :param expected_lines: list of strings to search for in each read line from the log file.
        :return: updated expected_lines list of strings after parsing the value of the line param.
        """
        expected_lines_to_remove = []

        for expected_line in expected_lines:
            searched_line = check_exact_match(line, expected_line)
            if expected_line == searched_line:
                logger.debug("Found expected line: {} from line: {}".format(expected_line, line))
                expected_lines_to_remove.append(expected_line)

        for expected_line in expected_lines_to_remove:
            expected_lines.remove(expected_line)

        return expected_lines

    def _find_unexpected_lines(self, line, unexpected_lines, halt_on_unexpected):
        """
        Checks for any matches between the 'line' string and strings in the 'unexpected_lines' list.
        Removes unexpected_lines strings that are found from the main unexpected_lines list and adds them to the
        unexpected_lines_found list.

        :param line: string from a TextIO or BinaryIO file object being read line by line.
        :param unexpected_lines: list of strings to search for in each read line from the log file.
        :param halt_on_unexpected: boolean to determine whether to raise ValueError on the first
            unexpected line found (True) or not (False)
        :return: unexpected_lines_found from the unexpected_lines searched for in the current log line.
        """
        unexpected_lines_found = self.unexpected_lines_found
        unexpected_lines_to_remove = []

        for unexpected_line in unexpected_lines:
            searched_line = check_exact_match(line, unexpected_line)
            if unexpected_line == searched_line:
                logger.debug("Found unexpected line: {} from line: {}".format(unexpected_line, line))
                if halt_on_unexpected:
                    raise LogMonitorException(
                        "Unexpected line appeared: {} from line: {}".format(unexpected_line, line))
                unexpected_lines_found.append(unexpected_line)
                unexpected_lines_to_remove.append(unexpected_line)

        for unexpected_line in unexpected_lines_to_remove:
            unexpected_lines.remove(unexpected_line)

        return unexpected_lines_found

    def _validate_results(self, expected_lines_not_found, unexpected_lines_found, expected_lines, unexpected_lines):
        """
        Parses the values in the expected_lines_not_found & unexpected_lines_found lists.
        If any expected lines were NOT found or unexpected lines WERE found, a LogMonitorException will be raised.
        The LogMonitorException message will detail the values that triggered the error.

        :param expected_lines_not_found: list of strings for expected lines that did NOT appeared in the log file.
        :param unexpected_lines_found: list of strings for unexpected lines that DID appear in the log file.
        :return: True if results are validated, but raises a LogMonitorException if any errors found.
        """
        failure_found = False
        fail_message = "While monitoring file '{}':\n".format(self.log_file_path)
        expected_line_failures = ''
        expected_lines_found = [line for line in expected_lines if line not in expected_lines_not_found]

        # Find out if any error strings need to be constructed.
        if expected_lines_not_found or unexpected_lines_found:
            failure_found = True

        # Add the constructed error strings and raise a LogMonitorException if any are found.
        expected_lines_info = ""
        for line in expected_lines:
            if line in expected_lines_found:
                expected_lines_info += "[ FOUND ] {}\n".format(line)
            else:
                expected_lines_info += "[ NOT FOUND ] {}\n".format(line)
            
        logger.info("LogMonitor Result:\n"
                    "--- Expected lines ---\n"
                    f"{expected_lines_info}"
                    f"Found {len(expected_lines_found)}/{len(expected_lines)} expected lines")
        
        if failure_found:
            if expected_lines_not_found:
                expected_line_failures = "\n".join(expected_lines_not_found)
                logger.error('Following expected lines *NOT FOUND*:\n{}'.format(expected_line_failures))
                fail_message += "Failed to find expected line(s):\n{}".format(expected_line_failures)
            if unexpected_lines_found:
                unexpected_line_failures = "\n".join(unexpected_lines_found)
                logger.error('Following unexpected lines *FOUND*:\n{}'.format(unexpected_line_failures))
                fail_message += "Additionally, " if expected_line_failures else ""
                fail_message += "Found unexpected line(s):\n{}".format(unexpected_line_failures)
                
            raise LogMonitorException(fail_message)
    
        return True

    def _find_lines(self, log, expected_lines, unexpected_lines, halt_on_unexpected):
        """
        Given a list of strings in expected_lines, unexpected_lines, and a log file, read every line in the log file,
        and make sure all expected_lines strings appear & no unexpected_lines strings appear in the log file.
        NOTE: This loop will only end when a launcher process ends or if used as a callback function (i.e. waiter).

        :param log: TextIO or BinaryIO file object to read lines from.
        :param expected_lines: list of strings to search for in each read line from the log file.
        :param unexpected_lines: list of strings that must not be present in the log_file_path file.
        :param halt_on_unexpected: boolean to determine whether to raise LogMonitorException on the first
            unexpected line found (True) or not (False)
        :return: (wait_condition) Whether the log processing has finished(True: finished, False: unfinished)
            sets self.unexpected_lines_found & self.expected_lines_not_found
        """
        log_filename = os.path.basename(self.log_file_path)

        def process_line(line):
            self.py_log += ("|%s| %s\n" % (log_filename, line))
            expected_lines_not_found = self._find_expected_lines(line, expected_lines)
            unexpected_lines_found = self._find_unexpected_lines(line, unexpected_lines, halt_on_unexpected)
            self.unexpected_lines_found = unexpected_lines_found
            self.expected_lines_not_found = expected_lines_not_found

        exception_info = None

        # To avoid race conditions, we will check *before reading*
        # If in the mean time the file is closed, we will make sure we read everything by issuing an extra call
        # by returning the previous alive state
        process_runing = self.launcher.is_alive() 
        for line in log:
            line = line[:-1]  # remove /n
            try:
                process_line(line)
            except LogMonitorException as e:
                if exception_info is None:
                    exception_info = e.args

        if exception_info is not None:
            raise LogMonitorException(*exception_info)

        return not process_runing  # Will loop until the process ends
