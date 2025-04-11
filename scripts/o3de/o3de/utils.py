#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
This file contains utility functions
"""
import argparse
import importlib.util
import logging
import os
import stat
import pathlib
import re
import shutil
import string
import sys
import urllib.request
from urllib.parse import ParseResult
import uuid
import zipfile
from packaging.specifiers import SpecifierSet

from o3de import github_utils, git_utils, validation as valid
from subprocess import Popen, PIPE, STDOUT
from typing import List, Tuple

LOG_FORMAT = '[%(levelname)s] %(name)s: %(message)s'
UI_MSG_FORMAT = '%(message)s'

logger = logging.getLogger('o3de.utils')
logging.basicConfig(format=LOG_FORMAT)

COPY_BUFSIZE = 64 * 1024


class VerbosityAction(argparse.Action):
    def __init__(self,
                 option_strings,
                 dest,
                 default=None,
                 required=False,
                 help=None):
        super().__init__(
            option_strings=option_strings,
            dest=dest,
            nargs=0,
            default=default,
            required=required,
            help=help,
        )

    def __call__(self, parser, namespace, values, option_string=None):
        count = getattr(namespace, self.dest, None)
        if count is None:
            count = self.default
        count += 1
        setattr(namespace, self.dest, count)
        # get the parent logger instance
        log = logging.getLogger('o3de')
        if count >= 2:
            log.setLevel(logging.DEBUG)
        elif count == 1:
            log.setLevel(logging.INFO)

class CLICommand(object):
    """
    CLICommand is an interface for storing CLI commands as list of string arguments to run later in a script.
    A current working directory, pre-existing OS environment, and desired logger can also be specified.
    To execute a command, use the run() function.
    This class is responsible for starting a new process, polling it for updates and logging, and safely terminating it.
    """
    def __init__(self, 
                args: list,
                cwd: pathlib.Path,
                logger: logging.Logger,
                env: os._Environ=None) -> None:
        self.args = args
        self.cwd = cwd
        self.env = env
        self.logger = logger
        self._stdout_lines = []
        self._stderr_lines = []
    
    @property
    def stdout_lines(self) -> List[str]:
        """The result of stdout, separated by newlines."""
        return self._stdout_lines

    @property
    def stdout(self) -> str:
        """The result of stdout, as a single string."""
        return "\n".join(self._stdout_lines)

    @property
    def stderr_lines(self) -> List[str]:
        """The result of stderr, separated by newlines."""
        return self._stderr_lines

    @property
    def stderr(self) -> str:
        """The result of stderr, as a single string."""
        return "\n".join(self._stderr_lines)

    
    def _poll_process(self, process) -> None:
        # while process is not done, read any log lines coming from subprocess
        while process.poll() is None:
            line = process.stdout.readline()
            if not line: break

            log_line = line.decode('utf-8', 'ignore').rstrip()
            self._stdout_lines.append(log_line)
            self.logger.info(log_line)
    
    def _cleanup_process(self, process) -> None:
        # flush remaining log lines
        log_lines = process.stdout.read().decode('utf-8', 'ignore')
        self._stdout_lines += log_lines.split('\n')
        self.logger.info(log_lines)

        safe_kill_processes(process, process_logger = self.logger)


    
    def run(self) -> int:
        """
        Takes the arguments specified during CLICommand initialization, and opens a new subprocess to handle it.
        This function automatically manages polling the process for logs, error reporting, and safely cleaning up the process afterwards.
        :return return code on success or failure 
        """
        ret = 1
        try:
            with Popen(self.args, cwd=self.cwd, env=self.env, stdout=PIPE, stderr=STDOUT) as process:
                self.logger.info(f"Running process '{self.args[0]}' with PID({process.pid}): {self.args}")
                
                self._poll_process(process)

                self._cleanup_process(process)

                ret = process.returncode

                return ret

        except Exception as err:
            self.logger.error(err)
            raise err


# Per Python documentation, only strings should be inserted into sys.path
# https://docs.python.org/3/library/sys.html#sys.path
def prepend_to_system_path(file_path: pathlib.Path or str) -> None:
    """
    Prepend the running script's imported system module paths. Useful for loading scripts in a foreign directory
    :param path: The file path of the desired script to load
    :return: None
    """
    if isinstance(file_path, str):
        file_path = pathlib.Path(file_path)

    folder_path = file_path if file_path.is_dir() else file_path.parent
    if str(folder_path) not in sys.path:
        sys.path.insert(0, str(folder_path))

def load_and_execute_script(script_path: pathlib.Path or str, **context_variables) -> int:
    """
    For a given python script, use importlib to load the script spec and module to execute it later
    :param script_path: The path to the python script to run
    :param context_variables: A series of keyword arguments which specify the context for the script before it is run.
    :return: return code indicating succes or failure of script
    """
    
    if isinstance(script_path, str):
        script_path = pathlib.Path(script_path)
    
    # load the target script as a module, set the context, and then execute
    script_name = script_path.name
    spec = importlib.util.spec_from_file_location(script_name, script_path)
    script_module = importlib.util.module_from_spec(spec)
    sys.modules[script_name] = script_module

    # inject the script module with relevant context variables
    for key, value in context_variables.items():
        setattr(script_module, key, value)

    try:
        spec.loader.exec_module(script_module)
    except Exception as err:
        logger.error(f"Failed to run script '{script_path}'. Here is the stacktrace: ", exc_info=True)
        return 1

    return 0

def add_verbosity_arg(parser: argparse.ArgumentParser) -> None:
    """
    Add a consistent/common verbosity option to an arg parser
    :param parser: The ArgumentParser to modify
    :return: None
    """
    parser.add_argument('-v', dest='verbosity', action=VerbosityAction, default=0,
                        help='Additional logging verbosity, can be -v or -vv')


def copyfileobj(fsrc, fdst, callback, length=0):
    # This is functionally the same as the python shutil copyfileobj but
    # allows for a callback to return the download progress in blocks and allows
    # to early out to cancel the copy.
    if not length:
        length = COPY_BUFSIZE

    fsrc_read = fsrc.read
    fdst_write = fdst.write

    copied = 0
    while True:
        buf = fsrc_read(length)
        if not buf:
            break
        fdst_write(buf)
        copied += len(buf)
        if callback(copied):
            return 1
    return 0

def remove_dir_path(path:pathlib.Path):
    """
    Helper function to delete a folder, ignoring all errors if possible
    :param path: The Path to the folder to delete
    """
    if path.exists() and path.is_dir():
        files_to_delete = []
        for root, dirs, files in os.walk(path):
            for file in files:
                files_to_delete.append(os.path.join(root, file))
        for file in files_to_delete:
            os.chmod(file, stat.S_IWRITE)
            os.remove(file)

        shutil.rmtree(path.resolve(), ignore_errors=True)


def validate_identifier(identifier: str) -> bool:
    """
    Determine if the identifier supplied is valid
    :param identifier: the name which needs to be checked
    :return: bool: if the identifier is valid or not
    """
    if not identifier:
        return False
    elif len(identifier) > 64:
        return False
    elif not identifier[0].isalpha():
        return False
    else:
        for character in identifier:
            if not (character.isalnum() or character == '_' or character == '-'):
                return False
    return True


def validate_version_specifier(version_specifier:str) -> bool:
    try:
        get_object_name_and_version_specifier(version_specifier)
    except (InvalidObjectNameException, InvalidVersionSpecifierException):
        return False
    return True 


def validate_version_specifier_list(version_specifiers:str or list) -> bool:
    version_specifier_list = version_specifiers.split() if isinstance(version_specifiers, str) else version_specifiers
    if not isinstance(version_specifier_list, list):
        logger.error(f'Version specifiers must be in the format <name><version specifiers>. e.g. name==1.2.3 \n {version_specifiers}')
        return False

    for version_specifier in version_specifier_list:
        if not validate_version_specifier(version_specifier):
            logger.error(f'Version specifiers must be in the format <name><version specifiers>. e.g. name==1.2.3 \n {version_specifier}')
            return False
    return True


def sanitize_identifier_for_cpp(identifier: str) -> str:
    """
    Convert the provided identifier to a valid C++ identifier
    :param identifier: the name which needs to be sanitized
    :return: str: sanitized identifier
    """
    cpp_keywords = [
        'alignas', 'constinit', 'false', 'public', 'true', 'alignof', 'const_cast', 'float', 'register',
        'try', 'asm', 'continue', 'for', 'reinterpret_cast', 'typedef', 'auto', 'co_await', 'friend',
        'requires', 'typeid', 'bool', 'co_return', 'goto', 'return', 'typename', 'break', 'co_yield',
        'if', 'short', 'union', 'case', 'decltype', 'inline', 'signed', 'unsigned', 'catch', 'default',
        'int', 'sizeof', 'using', 'char', 'delete', 'long', 'static', 'virtual', 'char8_t', 'do', 'mutable',
        'static_assert', 'void', 'char16_t', 'double', 'namespace', 'static_cast', 'volatile', 'char32_t',
        'dynamic_cast', 'new', 'struct', 'wchar_t', 'class', 'else', 'noexcept', 'switch', 'while', 'concept',
        'enum', 'nullptr', 'template', 'const', 'explicit', 'operator', 'this', 'consteval', 'export', 'private',
        'thread_local', 'constexpr', 'extern', 'protected', 'throw', 'and', 'and_eq', 'bitand', 'bitor', 'compl',
        'not', 'not_eq', 'or', 'or_eq', 'xor', 'xor_eq']
    
    if not identifier:
        return ''
    
    identifier_chars = list(identifier)
    for index, character in enumerate(identifier_chars):
        if not (character.isalnum() or character == '_'):
            identifier_chars[index] = '_'
            
    sanitized_identifier = "".join(identifier_chars)
    if sanitized_identifier in cpp_keywords:
        sanitized_identifier += '_'
            
    return sanitized_identifier


def validate_uuid4(uuid_string: str) -> bool:
    """
    Determine if the uuid supplied is valid
    :param uuid_string: the uuid which needs to be checked
    :return: bool: if the uuid is valid or not
    """
    try:
        val = uuid.UUID(uuid_string, version=4)
    except ValueError:
        return False
    return str(val) == uuid_string


def backup_file(file_name: str or pathlib.Path) -> None:
    index = 0
    renamed = False
    while not renamed:
        backup_file_name = pathlib.Path(str(file_name) + '.bak' + str(index)).resolve()
        index += 1
        if not backup_file_name.is_file():
            file_name = pathlib.Path(file_name).resolve()
            file_name.rename(backup_file_name)
            if backup_file_name.is_file():
                renamed = True


def backup_folder(folder: str or pathlib.Path) -> None:
    index = 0
    renamed = False
    while not renamed:
        backup_folder_name = pathlib.Path(str(folder) + '.bak' + str(index)).resolve()
        index += 1
        if not backup_folder_name.is_dir():
            folder = pathlib.Path(folder).resolve()
            folder.rename(backup_folder_name)
            if backup_folder_name.is_dir():
                renamed = True


def get_git_provider(parsed_uri: ParseResult):
    """
    Returns a git provider if one exists given the passed uri
    :param parsed_uri: uniform resource identifier of a possible git repository
    :return: A git provider implementation providing functions to get infomration about or clone a repository, see gitproviderinterface
    """
    # check for providers with unique APIs first
    git_provider = github_utils.get_github_provider(parsed_uri)

    if not git_provider:
        # fallback to generic git provider
        git_provider = git_utils.get_generic_git_provider(parsed_uri)

    return git_provider


def download_file(parsed_uri: ParseResult, download_path: pathlib.Path, force_overwrite: bool = False, object_name: str = "", download_progress_callback = None) -> int:
    """
    Download file
    :param parsed_uri: uniform resource identifier to zip file to download
    :param download_path: location path on disk to download file
    :param force_overwrite: force overwrites the local file if one exists
    :param object_name: name of the object being downloaded
    :param download_progress_callback: callback called with the download progress as a percentage, returns true to request to cancel the download
    """
    file_exists = download_path.is_file()

    if parsed_uri.scheme in ['http', 'https', 'ftp', 'ftps']:
        try:
            current_request = urllib.request.Request(parsed_uri.geturl())
            resume_position = 0
            if file_exists and not force_overwrite:
                resume_position = os.path.getsize(download_path)
                current_request.add_header("If-Range", "bytes=%d-" % resume_position)

            with urllib.request.urlopen(current_request) as s:
                download_file_size = int(s.headers.get('content-length',0))

                # if the server does not return a content length we also have to assume we would be replacing a complete file
                if file_exists and (resume_position == download_file_size or download_file_size == 0) and not force_overwrite:
                    logger.error(f'File already downloaded to {download_path} and force_overwrite is not set.')
                    return 1

                if s.getcode() == 206: # partial content
                    file_mode = 'ab'
                elif s.getcode() == 200:
                    file_mode = 'wb'
                else:
                    logger.error(f'HTTP status {e.code} opening {parsed_uri.geturl()}')
                    return 1
                
                # remove the file only after we have a response from the server and something to replace it with
                if file_exists and force_overwrite:
                    try:
                        os.unlink(download_path)
                    except OSError:
                        logger.error(f'Could not remove existing download path {download_path}.')
                        return 1

                def print_progress(downloaded, total_size):
                    end_ch = '\r'
                    if total_size == 0 or downloaded > total_size:
                        print(f'Downloading {object_name if object_name else parsed_uri.geturl()} - {downloaded} bytes')
                    else:
                        if downloaded == total_size:
                            end_ch = '\n'
                        print(f'Downloading {object_name if object_name else parsed_uri.geturl()} - {downloaded} of {total_size} bytes - {(downloaded/total_size)*100:.2f}%', end=end_ch)

                if download_progress_callback == None:
                    download_progress_callback = print_progress

                def download_progress(downloaded_bytes):
                    if download_progress_callback:
                        return download_progress_callback(int(downloaded_bytes), download_file_size)
                    return False

                with download_path.open(file_mode) as f:
                    download_cancelled = copyfileobj(s, f, download_progress)
                    if download_cancelled:
                        logger.info(f'Download of file to {download_path} cancelled.')
                        return 1
        except urllib.error.HTTPError as e:
            logger.error(f'HTTP Error {e.code} opening {parsed_uri.geturl()}')
            return 1
        except urllib.error.URLError as e:
            logger.error(f'URL Error {e.reason} opening {parsed_uri.geturl()}')
            return 1
    else:
        parsed_uri_path = urllib.parse.unquote(parsed_uri.path)
        if isinstance(download_path, pathlib.PureWindowsPath):
            # On Windows we want to remove the initial slash in front of the drive letter
            if parsed_uri_path.startswith('/'):
                parsed_uri_path = parsed_uri_path[1:]
            else:
                logger.warning(f"The provided path URI '{parsed_uri_path}' may be missing a '/', "
                               "file URIs typically have 3 slashes when they have an empty authority (RFC 8089) e.g. file:///")
        origin_file = pathlib.Path(parsed_uri_path).resolve()
        if not origin_file.is_file():
            logger.error(f"Failed to find local file '{origin_file}' based on URI '{parsed_uri}'")
            return 1
        shutil.copy(origin_file, download_path)

    return 0


def download_zip_file(parsed_uri, download_zip_path: pathlib.Path, force_overwrite: bool, object_name: str, download_progress_callback = None) -> int:
    """
    :param parsed_uri: uniform resource identifier to zip file to download
    :param download_zip_path: path to output zip file
    :param force_overwrite: force overwrites the local file if one exists
    :param object_name: name of the object being downloaded
    :param download_progress_callback: callback called with the download progress as a percentage, returns true to request to cancel the download
    """
    download_file_result = download_file(parsed_uri, download_zip_path, force_overwrite, object_name, download_progress_callback)
    if download_file_result != 0:
        return download_file_result

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"File zip {download_zip_path} is invalid. Try re-downloading the file.")
        download_zip_path.unlink()
        return 1

    return 0


def find_ancestor_file(target_file_name: pathlib.PurePath, start_path: pathlib.Path,
                       max_scan_up_range: int=0) -> pathlib.Path or None:
    """
    Find a file with the given name in the ancestor directories by walking up the starting path until the file is found
    :param target_file_name: Name of the file to find
    :param start_path: path to start looking for the file
    :param max_scan_up_range: maximum number of directories to scan upwards when searching for target file
           if the value is 0, then there is no max
    :return: Path to the file or None if not found
    """
    current_path = pathlib.Path(start_path)
    candidate_path = current_path / target_file_name

    max_scan_up_range = max_scan_up_range if max_scan_up_range else sys.maxsize

    # Limit the number of directories to traverse, to avoid infinite loop in path cycles
    for _ in range(max_scan_up_range):
        if candidate_path.exists():
            # Found the file we wanted
            break

        parent_path = current_path.parent
        if parent_path == current_path:
            # Only true when we are at the directory root, can't keep searching
            break
        candidate_path = parent_path / target_file_name
        current_path = parent_path

    return candidate_path if candidate_path.exists() else None


def find_ancestor_dir_containing_file(target_file_name: pathlib.PurePath, start_path: pathlib.Path,
                                      max_scan_up_range: int=0) -> pathlib.Path or None:
    """
    Find the nearest ancestor directory that contains the file with the given name by walking up
    from the starting path
    :param target_file_name: Name of the file to find
    :param start_path: path to start looking for the file
    :param max_scan_up_range: maximum number of directories to scan upwards when searching for target file
           if the value is 0, then there is no max
    :return: Path to the directory containing file or None if not found
    """
    ancestor_file = find_ancestor_file(target_file_name, start_path, max_scan_up_range)
    return ancestor_file.parent if ancestor_file else None

def get_project_path_from_file(target_file_path: pathlib.Path, supplied_project_path: pathlib.Path = None) -> pathlib.Path or None:
    """
    Based on a file supplied by the user, and optionally a differing project path, determine and validate a proper project path, if any.
    :param target_file_path: A user supplied file path
    :param supplied_project_path: (Optional) If the target file is different from the project path, the user may also specify this path as well.
    :return: A valid project path, or None
    """
    project_path = supplied_project_path
    if not project_path:
        project_path = find_ancestor_dir_containing_file(pathlib.PurePath('project.json'), target_file_path)
    
    if not project_path or not valid.valid_o3de_project_json(project_path / 'project.json'):
        return None

    return project_path

def get_gem_names_set(gems: list, include_optional:bool = True) -> set:
    """
    For working with the 'gem_names' lists in project.json
    Returns a set of gem names in a list of gems
    :param gems: The original list of gems, strings or small dicts (json objects)
    :param include_optional: If false, exclude optional gems
    :return: A set of gem name strings
    """
    def should_include_gem(gem: str or dict) -> bool:
        if not isinstance(gem, dict) or include_optional:
            return True
        else:
            # only include required gems 
            return not gem.get('optional', False)

    return set([gem['name'] if isinstance(gem, dict) else gem for gem in gems if should_include_gem(gem)])


def add_or_replace_object_names(object_names:set, new_object_names:list) -> list:
    """
    Returns a list of object names with optional version specifiers, where all objects in
    the object_names list are replaced with objects in the new_object_names list.  Any object_names, that 
    don't exist in object_names are added to the new list
    NOTE: this function only accepts lists of strings, it does not work with lists that contain dicts
    :param object_names: The set of object names with optional version specifiers
    :param new_object_names: The object names with optional version specifiers to add or replace in object_names 
    :return: the combined list of object_names modified with new_object_names 
    """
    object_name_map = {}
    for object_name_with_specifier in object_names:
        object_name, _ = get_object_name_and_optional_version_specifier(object_name_with_specifier)
        object_name_map[object_name] = object_name_with_specifier
    
    # overwrite or add objects from new_object_names
    for object_name_with_specifier in new_object_names:
        object_name, _ = get_object_name_and_optional_version_specifier(object_name_with_specifier)
        object_name_map[object_name] = object_name_with_specifier

    return object_name_map.values()


def contains_object_name(object_name:str, candidates:list) -> bool:
    """
    Returns True if any item in the list of candidates contains object_name with or
    without a version specifier
    :param object_name: The object name to search for 
    :param candidates: The list of candidate object names with optional version specifiers 
    :return: True if a match is found 
    """
    for candidate in candidates:
        candidate_name, _ = get_object_name_and_optional_version_specifier(candidate)
        if candidate_name == object_name:
            return True
    return False


def remove_gem_duplicates(gems: list) -> list:
    """
    For working with the 'gem_names' lists in project.json
    Adds names to a dict, and when a collision occurs, eject the existing one in favor of the new one.
    This is because when adding gems the list is extended, so the override will come last.
    :param gems: The original list of gems, strings or small dicts (json objects)
    :return: A new list with duplicate gem entries removed
    """
    new_list = []
    names = {}
    for gem in gems:
        if not (isinstance(gem, dict) or isinstance(gem, str)):
            continue
        gem_name = gem.get('name', '') if isinstance(gem, dict) else gem
        if gem_name:
            if gem_name not in names:
                names[gem_name] = len(new_list)
                new_list.append(gem)
            else:
                new_list[names[gem_name]] = gem
    return new_list


def update_keys_and_values_in_dict(existing_values: dict, new_values: list or str, remove_values: list or str,
                      replace_values: list or str):
    """
    Updates values within a dictionary by replacing all values or by appending values in the new_values list and 
    removing values in the remove_values
    :param existing_values dict to modify
    :param new_values list of key=value pairs to add to the existing dictionary 
    :param remove_values list with keys to remove from the existing dictionary 
    :param replace_values list with key=value pairs to replace existing dictionary with

    returns updated existing dictionary
    """
    if replace_values != None:
        replace_values = replace_values.split() if isinstance(replace_values, str) else replace_values
        return dict(entry.split('=') for entry in replace_values if '=' in entry)
    
    if new_values:
        new_values = new_values.split() if isinstance(new_values, str) else new_values
        new_values = dict(entry.split('=') for entry in new_values if '=' in entry)
        if new_values:
            existing_values.update(new_values)
    
    if remove_values:
        remove_values = remove_values.split() if isinstance(remove_values, str) else remove_values
        [existing_values.pop(key) for key in remove_values]
    
    return existing_values


def update_values_in_key_list(existing_values: list, new_values: list or str, remove_values: list or str,
                      replace_values: list or str):
    """
    Updates values within a list by replacing all values or by appending values in the new_values list, 
    removing values in the remove_values and then removing duplicates.
    :param existing_values list with existing values to modify
    :param new_values list with values to add to the existing value list
    :param remove_values list with values to remove from the existing value list
    :param replace_values list with values to replace in the existing value list

    returns updated existing value list
    """
    if replace_values != None:
        replace_values = replace_values.split() if isinstance(replace_values, str) else replace_values
        return list(dict.fromkeys(replace_values))

    if new_values:
        new_values = new_values.split() if isinstance(new_values, str) else new_values
        existing_values.extend(new_values)
    if remove_values:
        remove_values = remove_values.split() if isinstance(remove_values, str) else remove_values
        existing_values = list(filter(lambda value: value not in remove_values, existing_values))

    # replace duplicate values
    return list(dict.fromkeys(existing_values))


class InvalidVersionSpecifierException(Exception):
    pass


class InvalidObjectNameException(Exception):
    pass


def get_object_name_and_version_specifier(input:str) -> (str, str) or None:
    """
    Get the object name and version specifier from a string in the form <name><version specifier(s)>
    Valid input examples:
        o3de>=1.2.3
        o3de-sdk==1.2.3,~=2.3.4
    :param input a string in the form <name><PEP 440 version specifier(s)> where the version specifier includes the relational operator such as ==, >=, ~=

    return an engine name and a version specifier or raises an exception if input is invalid
    """

    regex_str = r"(?P<object_name>(.*?))(?P<version_specifier>((~=|==|!=|<=|>=|<|>|===)(\s*\S+)+))"
    regex = re.compile(regex_str, re.IGNORECASE)
    match = regex.fullmatch(input.strip())

    if not match:
        raise InvalidVersionSpecifierException(f"Invalid name and/or version specifier {input}, expected <name><version specifiers> e.g. o3de==1.2.3")

    if not match.group("object_name"):
        raise InvalidObjectNameException(f"Invalid or missing name {input}, expected <name><version specifiers> e.g. o3de==1.2.3")

    # SpecifierSet will raise an exception if invalid
    if not SpecifierSet(match.group("version_specifier")):
        return None
    
    return match.group("object_name").strip(), match.group("version_specifier").strip()


def object_name_found(input:str, match:str) -> bool:
    """
    Returns True if the object name in the input string matches match 
    :param input: The input string
    :param match: The object name to match
    """
    object_name, _ = get_object_name_and_optional_version_specifier(input)
    return object_name == match

def get_object_name_and_optional_version_specifier(input:str):
    """
    Returns an object name and optional version specifier 
    :param input: The input string
    """
    try:
        return get_object_name_and_version_specifier(input)
    except (InvalidObjectNameException, InvalidVersionSpecifierException):
        return input, None


def replace_dict_keys_with_value_key(input:dict, value_key:str, replaced_key_name:str = None, place_values_in_list:bool = False):
    """
    Takes a dictionary of dictionaries and replaces the keys with the value of 
    a specific value key.
    For example, if you have a dictionary of gem_paths->gem_json_data, this function can be used
    to convert the dictionary so the keys are gem names instead of paths (gem_name->gem_json_data)
    :param input: A dictionary of key->value pairs where every value is a dictionary that has a value_key
    :param value_key: The value's key to replace the current key with
    :param replaced_key_name: (Optional) A key name under which to store the replaced key in value
    :param place_values_in_list: (Optional) Put the values in a list, useful when the new key is not unique
    """

    # we cannot iterate over the dict while deleting entries
    # so we iterate over a copy of the keys
    keys = list(input.keys())
    for key in keys:
        value = input[key]

        # if the value is invalid just remove it
        if value == None:
            del input[key]
            continue

        # include the key we're removing if replaced_key_name provided
        if replaced_key_name:
            value[replaced_key_name] = key

        # remove the current entry 
        del input[key]

        # replace with an entry keyed on value_key's value
        if place_values_in_list:
            entries = input.get(value[value_key], [])
            entries.append(value)
            input[value[value_key]] = entries
        else:
            input[value[value_key]] = value

def safe_kill_processes(*processes: List[Popen], process_logger: logging.Logger = None) -> None:
    """
    Kills a given process without raising an error
    :param processes: An iterable of processes to kill
    :param process_logger: (Optional) logger to use
    """
    def on_terminate(proc) -> None:
        try:
            process_logger.info(f"process '{proc.args[0]}' with PID({proc.pid}) terminated with exit code {proc.returncode}")
        except Exception:  # purposefully broad
            process_logger.error("Exception encountered with termination request, with stacktrace:", exc_info=True)

    if not process_logger:
        process_logger = logger
    
    for proc in processes:
        try:
            process_logger.info(f"Terminating process '{proc.args[0]}' with PID({proc.pid})")
            proc.kill()
        except Exception:  # purposefully broad
            process_logger.error("Unexpected exception ignored while terminating process, with stacktrace:", exc_info=True)
    try:
        for proc in processes:
            proc.wait(timeout=30)
            on_terminate(proc)
    except Exception:  # purposefully broad
        process_logger.error("Unexpected exception while waiting for processes to terminate, with stacktrace:", exc_info=True)


def load_template_file(template_file_path: pathlib.Path, template_env: dict, read_encoding:str = 'UTF-8', encoding_error_action:str='ignore') -> str:
    """
    Helper method to load in a template file and return the processed template based on the input template environment
    This will also handle '###' tokens to strip out of the final output completely to support things like adding
    copyrights to the template that is not intended for the output text

    :param template_file_path:  The path to the template file to load
    :param template_env:        The template environment dictionary for the template file to process
    :param read_encoding:       The text encoding to use to read from the file
    :param  encoding_error_action:  The action to take on encoding errors
    :return:    The processed content from the template file
    :raises:    FileNotFoundError: If the template file path cannot be found
    """
    try:
        template_file_content = template_file_path.resolve(strict=True).read_text(encoding=read_encoding,
                                                                                  errors=encoding_error_action)
        # Filter out all lines that start with '###' before replacement
        filtered_template_file_content = (str(re.sub('###.*', '', template_file_content)).strip())

        return string.Template(filtered_template_file_content).substitute(template_env)
    except FileNotFoundError:
        raise FileNotFoundError(f"Invalid file path. Cannot find template file located at {str(template_file_path)}")


def remove_link(link:pathlib.PurePath):
    """
    Helper function to either remove a symlink, or remove a folder
    """
    link = pathlib.PurePath(link)
    if os.path.isdir(link):
        try:
            os.unlink(link)
        except OSError:
            # If unlink fails use shutil.rmtree
            def remove_readonly(func, path, _):
                # Clear the readonly bit and reattempt the removal
                os.chmod(path, stat.S_IWRITE)
                func(path)

            try:
                shutil.rmtree(link, onerror=remove_readonly)
            except shutil.Error as shutil_error:
                raise RuntimeError(f'Error trying remove directory {link}: {shutil_error}', shutil_error.errno)
