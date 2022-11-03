"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Small library of functions to support autotests for asset processor

"""

# Import builtin libraries
import pytest
import binascii
import hashlib
import os
import re
import hashlib
import shutil
import logging
import subprocess
import psutil
from configparser import ConfigParser
from typing import Dict, List, Tuple, Optional, Callable

# Import LyTestTools
import ly_test_tools.environment.file_system as fs
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools._internal.exceptions as exceptions
from ly_test_tools.o3de.ap_log_parser import APLogParser

logger = logging.getLogger(__name__)

# Asset Processor fast scan system setting key/subkey
AP_FASTSCAN_KEY = r"Software\O3DE\O3DE Asset Processor\Options"
AP_FASTSCAN_SUBKEY = r"EnableZeroAnalysis"


class ProcessOutput(object):
    # Process data holding object
    def __init__(self) -> None:
        # type() -> None
        self.stdout = None
        self.stderr = None
        self.returncode = None
        self.exception_occurred = False


def compare_assets_with_cache(assets: List[str], assets_cache_path: str) -> Tuple[List[str], List[str]]:
    """
    Given a list of assets names, will try to find them (disrespecting file extensions)
    from project's Cache folder with test assets

    :param assets: A list of assets to be compared with Cache
    :param assets_cache_path: A path to cache test assets folder
    :return: A tuple with two lists - first is missing in cache assets, second is existing in cache assets
    """
    missing_assets = []
    existing_assets = []
    if os.path.exists(assets_cache_path):
        files_in_cache = list(map(fs.remove_path_and_extension, os.listdir(assets_cache_path)))
        for asset in assets:
            file_without_ext = fs.remove_path_and_extension(asset).lower()
            if file_without_ext in files_in_cache:
                existing_assets.append(file_without_ext)
                files_in_cache.remove(file_without_ext)
            else:
                missing_assets.append(file_without_ext)
    else:
        missing_assets = assets
    return missing_assets, existing_assets


def copy_assets_to_project(assets: List[str], source_directory: str, target_asset_dir: str) -> None:
    """
    Given a list of asset names and a directory, copy those assets into the target project directory

    :param assets: A list of asset names to be copied
    :param source_directory: A path string where assets are located
    :param target_asset_dir: A path to project tests assets directory where assets will be copied over to
    :return: None
    """
    if not os.path.exists(target_asset_dir):
        os.mkdir(target_asset_dir)
    for asset in assets:
        full_name = os.path.join(source_directory, asset)
        destination_fullname = os.path.join(target_asset_dir, asset)
        if os.path.isdir(full_name):
            shutil.copytree(full_name, destination_fullname)
        else:
            shutil.copyfile(full_name, destination_fullname)
        os.chmod(destination_fullname, 0o0777)


def prepare_test_assets(assets_path: str, function_name: str, project_test_assets_dir: str) -> str:
    """
    Given function name and assets cache path, will clear cache and copy test assets assigned to function name to
    project's folder

    :param assets_path: Path to tests assets folder
    :param function_name: Name of a function that corresponds to folder with assets
    :param project_test_assets_dir: A path to project directory with test assets
    :return: Returning path to copied assets folder
    """
    test_assets_folder = os.path.join(assets_path, "assets", function_name)
    # Some tests don't have any assets to copy, which is fine, we don't want to fail in that case
    if os.path.exists(test_assets_folder):
        copy_assets_to_project(os.listdir(test_assets_folder), test_assets_folder, project_test_assets_dir)
    return test_assets_folder



def find_joblog_file(joblogs_path: str, regexp: str) -> str:
    """
    Given path to joblogs files and asset name in form of regexp, will try to find joblog file for provided asset;
    if multiple - will return first occurrence

    :param joblogs_path: Path to a folder with joblogs files to look for needed file
    :param regexp: Python Regexp to find the joblog file for the asset that was processed
    :return: Full path to joblog file, empty string if not found
    """
    for file_name in os.listdir(joblogs_path):
        if re.match(regexp, file_name):
            return os.path.join(joblogs_path, file_name)
    return ""


def find_missing_lines_in_joblog(joblog_location: str, strings_to_verify: List[str]) -> List[str]:
    """
    Given joblog file full path and list of strings to verify, will find all missing strings in the file

    :param joblog_location: Full path to joblog file
    :param strings_to_verify: List of string to look for in joblog file
    :return: Subset of original strings list, that were not found in the file
    """
    lines_not_found = []
    with open(joblog_location, "r") as f:
        read_data = f.read()
    for line in strings_to_verify:
        if line not in read_data:
            lines_not_found.append(line)
    return lines_not_found


def clear_project_test_assets_dir(test_assets_dir: str) -> None:
    """
    On call - deletes test assets dir if it exists and creates new empty one

    :param test_assets_dir: A path to tests assets dir
    :return: None
    """
    if os.path.exists(test_assets_dir):
        fs.delete([test_assets_dir], False, True)
    os.mkdir(test_assets_dir)


def get_files_hashsum(path_to_files_dir: str) -> Dict[str, int]:
    """
    On call - calculates md5 hashsums for filecontents.

    :param path_to_files_dir: A path to files directory
    :return: Returns a dict with initial filenames from path_to_files_dir as keys and their contents hashsums as values
    """
    checksum_dict = {}
    try:
        for fname in os.listdir(path_to_files_dir):
            with open(os.path.join(path_to_files_dir, fname), "rb") as fopen:
                checksum_dict[fname] = hashlib.sha256(fopen.read()).digest()
    except IOError:
        logger.error("An error occurred in LyTestTools when trying to read file.")
    return checksum_dict


def append_to_filename(file_name: str, path_to_file: str, append_text: str, ignore_extension: str) -> None:
    """
    Function for appending text to file and folder names

    :param file_name: Name of a file or folder
    :param path_to_file: Path to file or folder
    :param append_text: Text to append
    :param ignore_extension: True or False for ignoring extensions
    :return: None
    """
    if not ignore_extension:
        (name, extension) = file_name.split(".")
        new_name = name + append_text + "." + extension
    else:
        new_name = file_name + append_text
    os.rename(os.path.join(path_to_file, file_name), os.path.join(path_to_file, new_name))


def create_asset_processor_backup_directories(backup_root_directory: str, test_backup_directory: str) -> None:
    """
    Function for creating the asset processor logs backup directory structure

    :param backup_root_directory: The location where logs should be stored
    :param test_backup_directory: The directory for the specific test being ran
    :return: None
    """
    if not os.path.exists(os.path.join(backup_root_directory, test_backup_directory)):
        os.makedirs(os.path.join(backup_root_directory, test_backup_directory))


def backup_asset_processor_logs(bin_directory: str, backup_directory: str) -> None:
    """
    Function for backing up the logs created by asset processor to designated backup directory

    :param bin_directory: The bin directory created by the lumberyard build process
    :param backup_directory: The location where asset processor logs should be backed up to
    :return: None
    """
    ap_logs = os.path.join(bin_directory, "logs")

    if os.path.exists(ap_logs):
        destination = os.path.join(backup_directory, "logs")
        shutil.copytree(ap_logs, destination)


def safe_subprocess(command: str or List[str], **kwargs: Dict) -> ProcessOutput:
    """
    Forwards arguments to subprocess.Popen to have a processes output
    args stdout and stderr can not be passed as they are used internally
    Setting check = true will change the received out put into a subprocess.CalledProcessError object
    IMPORTANT: This code might fail after upgrade to python 3 due to interpretation of byte and string data.

    :param command: A list of the command to execute and its arguments as split by whitespace.
    :param kwargs: Keyword args forwarded to subprocess.check_output.
    :return: Popen object with callable attributes that hold the piped out put of the process.
    """

    cmd_string = command
    if type(command) == list:
        cmd_string = " ".join(command)

    logger.info(f'Executing "subprocess.Popen({cmd_string})"')
    # Initialize ProcessOutput object
    subprocess_output = ProcessOutput()
    try:
        # Run process
        # fmt:off
        output = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                  universal_newlines=True, **kwargs)
        # fmt:on

        # Wait for process to complete
        output_data = output.communicate()
        # Read and process pipped outputs
        subprocess_output.stderr = output_data[1]
        subprocess_output.stdout = output_data[0]
        # Save process return code
        subprocess_output.returncode = output.returncode
    except subprocess.CalledProcessError as e:
        # Set object flag
        subprocess_output.exception_occurred = True
        # If error occurs when **kwargs includes check=True Exceptions are possible
        logger.warning(f'Command "{cmd_string}" failed in LyTestTools with returncode {e.returncode}, output:\n{e.output}')
        # Read and process error outputs
        subprocess_output.stderr = e.output.read().decode()
        # Save error return code
        subprocess_output.returncode = e.returncode
    else:
        logger.info(f'Successfully executed "check_output({cmd_string})"')
    return subprocess_output


def processes_with_substring_in_name(substring: str) -> tuple:
    """
    Finds all existing processes that contain a specified substring in their names

    :param substring: the string to look for as a substring within process names
    :return: a tuple of all processes containing the substring in their names or an empty tuple if none are found
    """
    processes = process_utils._safe_get_processes()
    targeted_processes = []
    for p in processes:
        try:
            if substring.lower() in p.name().lower():
                targeted_processes.append(p)
        except psutil.NoSuchProcess as e:
            logger.info(f"Process {p} was killed in LyTestTools during processes_with_substring_in_name()!\nError: {e}")
            continue
    return tuple(targeted_processes)


def child_process_list(pid: int, name_filter: str = None) -> List[int]:
    """
    Return the list of child process objects of the given pid

    :param pid: process id of the parent process
    :param name_filter: optional name to match child processes against
    :return: List of matching process objects
    """
    return_list = []
    for child in psutil.Process(pid).children(recursive=True):
        if not name_filter or child.name() == name_filter:
            return_list.append(child)
    return return_list


def process_cpu_usage_below(process_name: str, cpu_usage_threshold: float) -> bool:
    """
    Checks whether CPU usage by a specified process is below a specified threshold

    :param process_name: String to search for within the names of active processes
    :param cpu_usage_threshold: Float at or above which CPU usage by a process instance is too high
    :return: True if the CPU usage for each instance of the specified process is below the threshold, False if not
    """
    # Get all instances of targeted process
    targeted_processes = processes_with_substring_in_name(process_name)
    if not len(targeted_processes) > 0:
        raise exceptions.LyTestToolsFrameworkException(f"No instances of {process_name} were found")

    # Return whether all instances of targeted process are idle
    for targeted_process in targeted_processes:
        logger.info(f"Process name: {targeted_process.name()}")
        if hasattr(targeted_process, "pid"):
            logger.info(f"Process ID: {targeted_process.pid}")
        process_cpu_load = targeted_process.cpu_percent(interval=1)
        logger.info(f"Process CPU load: {process_cpu_load}")
        if process_cpu_load >= cpu_usage_threshold:
            return False
    return True


def temp_test_dir(request, dir_path: str) -> str:
    """
    Creates a temporary test directory to be deleted on teardown

    :param dir_path: path for the temporary test directory
    :return: path to the temporary test directory
    """
    # Clear the directory if it exists and create the temporary test directory
    clear_project_test_assets_dir(dir_path)

    # Delete the directory on teardown
    request.addfinalizer(lambda: fs.delete([dir_path], False, True))

    return dir_path


def get_relative_file_paths(start_dir: str, ignore_list: Optional[List[str]] = None) -> List[str]:
    """
    Collects all relative paths for files under the [start_dir] directory tree.
    Ignores a path if it contains any string in the [ignore_list].
    """
    if ignore_list is None:
        ignore_list = []
    all_files = []
    for root, _, files in os.walk(start_dir):
        for file_name in files:
            full_path = os.path.join(root, file_name)
            if all([False for word in ignore_list if word in full_path]):
                all_files.append(os.path.relpath(full_path, start_dir))
    return all_files


def compare_lists(actual: List[str], expected: List[str]) -> bool:
    """Compares the two lists of strings. Returns false and prints any discrepancies if present."""

    # Find difference between expected and actual
    diff = {"actual": [], "expected": []}
    for asset in actual:
        if asset not in expected:
            diff["actual"].append(asset)
    for asset in expected:
        if asset not in actual:
            diff["expected"].append(asset)

    # Log difference between actual and expected (if any). Easier for troubleshooting
    if diff["actual"]:
        logger.info("The following assets were actually found but not expected:")
        for asset in diff["actual"]:
            logger.info("   " + asset)
    if diff["expected"]:
        logger.info("The following assets were expected to be found but were actually not:")
        for asset in diff["expected"]:
            logger.info("   " + asset)

    # True ONLY IF both diffs are empty
    return not diff["actual"] and not diff["expected"]


def delete_MoveOutput_folders(search_path: List[str] or str) -> None:
    """
    Deletes any directories that start with 'MoveOutput' in specified search location

    :param search_path: either a single path or a list of paths to search inside for MoveOutput folders
    :return: None
    """
    delete_list = []

    def search_one_path(search_location):
        nonlocal delete_list

        for file_or_folder in os.listdir(search_location):
            file_or_folder_path = os.path.join(search_location, file_or_folder)
            if os.path.isdir(file_or_folder_path) and file_or_folder.startswith("MoveOutput"):
                delete_list.append(file_or_folder_path)

    if isinstance(search_path, List):
        for single_path in search_path:
            search_one_path(single_path)
    else:
        search_one_path(search_path)

    fs.delete(delete_list, False, True)


def find_queries(line: str, queries_to_find: List[str or List[str]]) -> List[str or List[str]]:
    """
    Searches for strings and/or combinations of strings within a line

    :param line: Line to search
    :param queries_to_find: List containing strings and/or lists of strings to find
    :return: List of strings and/or lists of strings found within the line
    """
    queries_found = []

    for query in queries_to_find:
        # If query is a list then find each list item as a substring within the line
        if isinstance(query, list):
            subqueries_to_find = query[:]
            while subqueries_to_find and subqueries_to_find[0] in line:
                subqueries_to_find.pop(0)
                if subqueries_to_find == []:
                    queries_found.append(query)
        # Otherwise find query as a substring within the line
        elif query in line:
            queries_found.append(query)

    return queries_found


def validate_log_output(
    log_output: List[str or List[str]],
    expected_queries: List[str or List[str]] = [],
    unexpected_queries: List[str or List[str]] = [],
    failure_cb: Callable = None
) -> None:
    """
    Asserts that the log output contains all expected queries and no unexpected queries.

    :param log_output: log output of the application
    :param expected_queries: String or list containing strings and/or lists of strings to be found
    :param unexpected_queries: String or list containing strings and/or lists of strings not to be found
    :param failure_cb: Optional callback when log output isn't expected, useful for printing debug info
    :return: None
    """
    expected_queries = [expected_queries] if isinstance(expected_queries, str) else expected_queries
    unexpected_queries = [unexpected_queries] if isinstance(unexpected_queries, str) else unexpected_queries
    unexpectedly_found = []

    for line in log_output:
        # Remove queries expectedly found in the log from queries to expect
        for found_query in find_queries(line, expected_queries):
            expected_queries.remove(found_query)
        # Save unexpectedly found lines
        if find_queries(line, unexpected_queries):
            unexpectedly_found.append(line)

    if failure_cb and (len(unexpected_queries) > 0 or len(expected_queries) > 0):
        failure_cb()

    # Assert no unexpected lines found and all expected queries found
    assert unexpectedly_found == [], f"Unexpected line(s) were found in the log run: {unexpectedly_found}"
    assert expected_queries == [], f"Expected query(s) were not found in the log run: {expected_queries}"


def validate_log_messages(
    log_file: str,
    expected_queries: str or List[str or List[str]] = [],
    unexpected_queries: str or List[str or List[str]] = [],
) -> None:
    """
    Asserts that the most recent log run contains all expected queries and no unexpected queries. Queries can be strings
    and/or combinations of strings [[using nested lists]].

    :param log_file: Path to the log file being used
    :param expected_queries: String or list containing strings and/or lists of strings to be found
    :param unexpected_queries: String or list containing strings and/or lists of strings not to be found
    :return: None
    """

    # Search the log lines in the latest log run
    validate_log_output(APLogParser(log_file).runs[-1]["Lines"], expected_queries, unexpected_queries)


def validate_relocation_report(
    log_file: str,
    expected_queries: str or List[str or List[str]] = [],
    unexpected_queries: str or List[str or List[str]] = [],
) -> None:
    """
    Asserts that the relocation report section of the most recent log run contains all expected queries and no
    unexpected queries. Queries can be strings and/or combinations of strings [[using nested lists]].

    :param log_file: Path to the log file being used
    :param expected_queries: String or list containing strings and/or lists of strings to be found
    :param unexpected_queries: String or list containing strings and/or lists of strings not to be found
    :return: None
    """
    expected_queries = [expected_queries] if isinstance(expected_queries, str) else expected_queries
    unexpected_queries = [unexpected_queries] if isinstance(unexpected_queries, str) else unexpected_queries
    unexpectedly_found = []
    in_relocation_report = False

    # Search the log lines which appear between opening and closing RELOCATION REPORT lines in the latest log run
    for line in APLogParser(log_file).runs[-1]["Lines"]:
        if "RELOCATION REPORT" in line:
            in_relocation_report = not in_relocation_report
            continue  # Go to next log line

        if in_relocation_report:
            # Remove queries expectedly found in the relocation report from queries to expect
            for found_query in find_queries(line, expected_queries):
                expected_queries.remove(found_query)
            # Save unexpectedly found lines
            if find_queries(line, unexpected_queries):
                unexpectedly_found.append(line)

    # Assert no unexpected lines found and all expected queries found
    assert unexpectedly_found == [], f"Unexpected line(s) were found in the relocation report: {unexpectedly_found}"
    assert expected_queries == [], f"Expected query(s) were not found in the relocation report: {expected_queries}"


def get_paths_from_wildcard(root_path: str, wildcard_str: str) -> List[str]:
    """
    Convert a wildcard path into a list of existing full paths.

    :param root_path: Full path in which to search for matches for the wildcard string
    :param wildcard_str: Must contain exactly one "*", at the beginning or end, or be simply "*"
    :return: List of all full paths which satisfy the wildcard criteria
    """
    rel_path_list = get_relative_file_paths(root_path)
    if not wildcard_str == "*":
        if wildcard_str.startswith("*"):
            rel_path_list = [item for item in rel_path_list if item.endswith(wildcard_str[1:])]
        elif wildcard_str.endswith("*"):
            rel_path_list = [item for item in rel_path_list if item.startswith(wildcard_str[0:-1])]
    return [os.path.join(root_path, item) for item in rel_path_list]


def check_for_perforce(error_on_no_perforce=True):
    command_list = ['p4', 'info']
    try:
        p4_output = subprocess.check_output(command_list).decode('utf-8')
    except subprocess.CalledProcessError as e:
        if error_on_no_perforce:
            logger.error(f"Failed to call {command_list} in LyTestTools with error {e}")
        return False
    except FileNotFoundError as e:
        if error_on_no_perforce:
            logger.error(f"Failed to call {command_list} in LyTestTools with error {e}")
        return False

    if not p4_output.startswith("User name:"):
        if error_on_no_perforce:
            logger.warning(f"Perforce not found, output was {p4_output}")
        return False

    client_root_match = re.search(r"Client root: (.*)\r", p4_output)
    if client_root_match is None:
        if error_on_no_perforce:
            logger.warning(f"Could not determine client root for p4 workspace. Perforce output was {p4_output}")
        return False
    else:
        # This requires the tests to be in the Perforce path that the tests run against.
        working_path = os.path.realpath(__file__).replace("\\", "/").lower()
        client_root = client_root_match.group(1).replace("\\", "/").lower()
        if not working_path.startswith(client_root):
            if error_on_no_perforce:
                logger.error(f"""Perforce client root '{client_root}' does not contain current test directory '{working_path}'.
                            Please run this test with a Perforce workspace that contains the test asset directory path.""")
            return False

    logger.info(f"Perforce found, output was {p4_output}")
    return True


def get_file_hash(filePath, hashBufferSize = 65536):
    if not os.path.exists(filePath):
        raise exceptions.LyTestToolsFrameworkException(f"Cannot get file hash, file at path '{filePath}' does not exist.")

    sha1 = hashlib.sha1()
    with open(filePath, 'rb') as cacheFile:
        while True:
            data = cacheFile.read(hashBufferSize)
            if not data:
                break
            sha1.update(data)
    return sha1.hexdigest()
