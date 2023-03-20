"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import re
from typing import List, Optional, Dict, Generator, Tuple
import time

logger = logging.getLogger(__name__)


class APOutputParser:
    """
    Asset Processor Parser to extract information from generic asset processor output.
    Immutable.
    """

    _SEPARATOR = ": "
    _NONE_LINE = "none"
    # Does support multiple output runs
    _NEW_RUN_LINE = "AzFramework File Logging New Run"

    # Regular expression constants and keys used for looking up information later.
    _RE_INT_LINES = (  # Extract an integer
        (re.compile(r"^Number of Assets Successfully Processed: (\d*)."), "Successes"),
        (re.compile(r"^Number of Assets Failed to Process: (\d*)."), "Failures"),
        (re.compile(r"^Number of Warnings Reported: (\d*)."), "Warnings"),
        (re.compile(r"^Number of Errors Reported: (\d*)."), "Errors"),
        (re.compile(r"^Control Port: (\d*)"), "Control Port"),
        (re.compile(r"^Listening Port: (\d*)"), "Listening Port"),
    )
    _RE_FLOAT_LINES = ((re.compile(r"^Total Assets Processing Time: (\d*\.?\d*)s"), "Time"),)  # Extract a float
    _RE_TUPLE_LINES = (  # Extract a tuple of integers
        (re.compile(r"^Builder optimization: (\d*) / (\d*)"), "Full Analysis"),
    )
    _RE_STR_LINES = (  # Extract a string
        (re.compile(r"^AssetBuilder: Source = (.*)"), "Source"),
        (re.compile(r"^AssetBuilder: Platforms = (.*)"), "Platforms"),
    )

    # JobLogs log both errors and warnings on the same line. Handled separately
    _RE_ERRORS_WARNINGS = re.compile(r"^S: (\d*)\D*(\d*)")

    def __init__(self, raw_output: str) -> None:
        self._runs = []

        self.log_type = "Output"
        self._parse_lines(raw_output)

    @property
    def file_path(self) -> str:
        """Parsed log file path"""
        return self._file_path

    @property
    def runs(self) -> List[Dict]:
        """
        Returns all runs from the log as a list of dictionaries.
        Dictionary keys are as follows:
            ["Lines"]: [str] - All lines from the log for that run.
            ["Timestamps"]: [int] - Timestamp from each entry in Lines list
            ["Errors"]: int - the number of errors produced.
            ["Warnings"]: int - the number of warnings produced.
            ["Successes"]: int - the recorded number of successful assets processed.
            ["Failures"]: int - the recorded number of assets that failed to process.
            ["Control Port"]: int - port for the control socket recorded in the ap gui log
            ["Listening Port"]: int - port asset processor is listening for incoming connections on,
                written to the AP log for both batch and gui
            ["Time"]: float - the recorded time it took for the AP to complete the run.
            ["Full Analysis"]: (int, int) - A tuple where (x, y) is associated with the line:
                    "x / y files required full analysis..."
            ["Source"]: str - (JobLogs only) the source recorded for the build job.
            ["Platforms"]: str - (JobLogs only) The platforms recorded for the job.
        """
        return self._runs

    def get_line_type(self, line: str) -> str:
        """Parses the line type from a trimmed log line. See: APLogParser._trim_line(self, line)"""
        split = line.split(self._SEPARATOR, 1)
        if len(split) > 1:
            return split[0]
        return ""

    def remove_line_type(self, line: str) -> str:
        """Removes the line type from a trimmed log line. See: APLogParser._trim_line(self, line)"""
        split = line.split(self._SEPARATOR, 1)
        if len(split) > 1:
            return split[1]
        return ""

    # fmt:off
    def get_lines(self, run: Optional[int], contains: Optional[List[str] or str] = None,
                  regex: Optional[str] = None) -> Generator[str, None, None] or Generator[re.Match, None, None]:
        # fmt:on
        """
        Iterate the lines in the log by specifying a run index (or None for all).
        Filter results returned using the [contains] string.
        Or return a regex match object via the [regex] string.
        Prioritizes [regex] searching over [contains] searching.

        :param run: The index of the run to search. If None, all runs are searched
        :param contains: A string (or list of strings) to search for in the log
        :param regex: A regular expression string to use to search the log.
        :return: Each line that matches
        """
        runs_to_search = []
        if run is None:
            runs_to_search = self._runs
        else:
            runs_to_search.append(self._runs[run])
        for run in runs_to_search:
            for line in run["Lines"]:
                if regex is not None:
                    match = re.match(regex, line)
                    if match:
                        yield match
                elif contains is not None:
                    if type(contains) == str:
                        contains = [contains]

                    # List comprehension returns empty list if all search strings are in the line
                    if not [True for search_string in contains if search_string not in line]:
                        yield line
                else:
                    yield line

    def _parse_lines(self, all_lines: str):
        current_run = self._create_log_dict()
        for i, line in enumerate(all_lines):
            trimmed, timestamp = self._trim_line(line)
            line_type = self.get_line_type(trimmed)
            if trimmed and line_type:
                if line_type != self._NONE_LINE:
                    # not a "None" line, digest the line
                    self._digest_line(trimmed, current_run, timestamp)
                elif self._NEW_RUN_LINE in all_lines[i - 1] and current_run["Lines"]:
                    # Hit the end of a "run" in the log. Store it, clear it and continue
                    self._runs.append(current_run)
                    current_run = self._create_log_dict()
        if current_run["Lines"]:
            self._runs.append(current_run)

    def _trim_line(self, line: str) -> str:

        """ For raw input in APOutputParser simply returns the line (Already "trimmed")
        APLogParser's implementation trims a raw log line to remove the first 3 sections when using a log
        See APLogParser : _trim_line below
        """
        return line, int(round(time.time() * 1000))

    def _digest_line(self, trimmed_line: str, current_run: Dict, line_timestamp: int) -> None:
        """Extracts relevant information (if present) and adds line to list of all lines"""
        line_type = self.get_line_type(trimmed_line)
        # Store all lines under ["Lines"]
        if line_type in current_run.keys():
            current_run[line_type].append(trimmed_line.split(self._SEPARATOR)[1])
        current_run["Lines"].append(trimmed_line)
        current_run["Timestamps"].append(line_timestamp)
        trimmed_line = self.remove_line_type(trimmed_line)

        # Parse useful data if present. Parsing rules declared in static constants.
        for pattern, run_key in self._RE_INT_LINES:
            # Look for integer data (Errors, warnings... etc.)
            result = pattern.match(trimmed_line)
            if result:
                current_run[run_key] = int(result.groups()[0])
                return
        for pattern, run_key in self._RE_FLOAT_LINES:
            # Look for float data (Time... etc.)
            result = pattern.match(trimmed_line)
            if result:
                current_run[run_key] = float(result.groups()[0])
                return
        for pattern, run_key in self._RE_TUPLE_LINES:
            # Look for tuple data ("x / y files required full analysis"... etc.)
            result = pattern.match(trimmed_line)
            if result:
                current_run[run_key] = (int(result.groups()[0]), int(result.groups()[1]))
                return
        for pattern, run_key in self._RE_STR_LINES:
            # Look for string data (Source, Platform... etc.)
            result = pattern.match(trimmed_line)
            if result:
                current_run[run_key] = result.groups()[0]
                return
        # JobLog Errors /  Warnings are reported on a single line. Handle separately
        result = self._RE_ERRORS_WARNINGS.match(trimmed_line)
        if result and len(result.groups()) == 2:
            current_run["Errors"] = int(result.groups()[0])
            current_run["Warnings"] = int(result.groups()[1])

    @staticmethod
    def _create_log_dict() -> Dict:
        """Creates a dictionary ready to used to store log information"""
        return {
            "Lines": [],
            "Errors": None,
            "Successes": None,
            "Warnings": None,
            "Time": None,
            "Full Analysis": None,
            "Source": None,
            "Platforms:": None,
            "Timestamps": []
        }


class APLogParser(APOutputParser):
    """
    Asset Processor Log Parser to extract information from an asset processor log.
    Immutable.
    """

    _SEPARATOR = "~~"

    _NONE_LINE = "none"
    _NEW_RUN_LINE = "AzFramework File Logging New Run"

    def __init__(self, file_path: str, raw_output: str = None) -> None:
        self._runs = []
        self._file_path = file_path
        self.log_type = None
        if "JobLogs" in file_path:
            self.log_type = "JobLog"
        elif "AP_Batch" in file_path:
            self.log_type = "Batch"
        elif "AP_GUI" in file_path:
            self.log_type = "GUI"

        self._parse_file()

    @property
    def file_path(self) -> str:
        """Parsed log file path"""
        return self._file_path

    def _parse_file(self) -> None:
        """
        Parses the APLogParser's file and populates a "run" for every AP logging run present.
        """
        try:
            logger.info(f"Parsing log file file: {self._file_path}")
            with open(self._file_path, "r") as log_file:
                all_lines = log_file.readlines()
                self._parse_lines(all_lines)
        except OSError:
            logger.error(f"Error opening file in LyTestTools at path: {self._file_path}")
            self._runs = []

    def _trim_line(self, line: str) -> Tuple[str, int]:

        """Trims a raw log line to remove the first 3 sections
        example:
            ~~1581617216532~~1~~0000000000002540~~AssetProcessor~~Loading (Gem) Module 'GemRegistry'...
        trims to:
            AssetProcessor~~Loading (Gem) Module 'GemRegistry'...
        where the trimmed line is: <line-type>~~<line-contents>
        """
        split = line.strip().split(self._SEPARATOR, 4)
        if len(split) > 4:
            return split[4], int(split[1])
        return "", 0
