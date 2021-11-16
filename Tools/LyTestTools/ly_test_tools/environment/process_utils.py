"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Process management functions, to supplement normal use of psutil and subprocess
"""
import logging
import os
import psutil
import subprocess
import ctypes

import ly_test_tools
import ly_test_tools.environment.waiter as waiter

logger = logging.getLogger(__name__)
_PROCESS_OUTPUT_ENCODING = 'utf-8'

# Default list of processes names to kill
LY_PROCESS_KILL_LIST = [
    'AssetBuilder', 'AssetProcessor', 'AssetProcessorBatch',
    'CrySCompileServer', 'Editor',
    'Profiler', 'RemoteConsole',
    'rc'  # Resource Compiler
]


def kill_processes_named(names, ignore_extensions=False):
    """
    Kills all processes with a given name

    :param names: string process name, or list of strings of process name
    :param ignore_extensions: ignore trailing file extensions. By default 'abc.exe' will not match 'abc'. Note that
        enabling this will cause 'abc.exe' to match 'abc', 'abc.bat', and 'abc.sh', though 'abc.GameLauncher.exe'
        will not match 'abc.DedicatedServer'
    """
    if not names:
        return

    name_set = set()
    if isinstance(names, str):
        name_set.add(names)
    else:
        name_set.update(names)

    if ignore_extensions:
        # both exact matches and extensionless
        stripped_names = set()
        for name in name_set:
            stripped_names.add(_remove_extension(name))
        name_set.update(stripped_names)

    # remove any blank names, which may empty the list
    name_set = set(filter(lambda x: not x.isspace(), name_set))
    if not name_set:
        return

    logger.info(f"Killing all processes named {name_set}")
    process_set_to_kill = set()
    for process in _safe_get_processes(['name', 'pid']):
        try:
            proc_name = process.name()
        except psutil.AccessDenied:
            logger.warning(f"Process {process} permissions error during kill_processes_named()", exc_info=True)
            continue
        except psutil.ProcessLookupError:
            logger.debug(f"Process {process} could not be killed during kill_processes_named() and was likely already "
                         f"stopped", exc_info=True)
            continue
        except psutil.NoSuchProcess:
            logger.debug(f"Process '{process}' was active when list of processes was requested but it was not found "
                         f"during kill_processes_named()", exc_info=True)
            continue

        if proc_name in name_set:
            logger.debug(f"Found process with name {proc_name}.")
            process_set_to_kill.add(process)

        if ignore_extensions:
            extensionless_name = _remove_extension(proc_name)
            if extensionless_name in name_set:
                process_set_to_kill.add(process)

    if process_set_to_kill:
        _safe_kill_processes(process_set_to_kill)


def kill_processes_started_from(path):
    """
    Kills all processes started from a given directory or executable

    :param path: path to application or directory
    """
    logger.info(f"Killing processes started from '{path}'")
    if os.path.exists(path):
        process_list = []
        for process in _safe_get_processes():
            try:
                process_path = process.exe()
            except (psutil.AccessDenied, psutil.NoSuchProcess):
                continue

            if process_path.lower().startswith(path.lower()):
                process_list.append(process)

        _safe_kill_processes(process_list)
    else:
        logger.warning(f"Path:'{path}' not found")


def kill_processes_with_name_not_started_from(name, path):
    """
    Kills all processes with a given name that NOT started from a directory or executable

    :param name: name of application to look for
    :param path: path where process shouldn't have started from
    """
    path = os.path.join(os.getcwd(), os.path.normpath(path)).lower()
    logger.info(f"Killing processes with name:'{name}' not started from '{path}'")
    if os.path.exists(path):
        proccesses_to_kill = []
        for process in _safe_get_processes(["name", "pid"]):
            try:
                process_path = process.exe()
            except (psutil.AccessDenied, psutil.NoSuchProcess) as ex:
                continue

            process_name = os.path.splitext(os.path.basename(process_path))[0]

            if process_name == os.path.basename(name) and not os.path.dirname(process_path.lower()) == path:
                logger.info("%s -> %s" % (os.path.dirname(process_path.lower()), path))
                proccesses_to_kill.append(process)

        _safe_kill_processes(proccesses_to_kill)
    else:
        logger.warning(f"Path:'{path}' not found")


def kill_process_with_pid(pid, raise_on_missing=False):
    """
    Kills the process with the specified pid

    :param pid: the pid of the process to kill
    :param raise_on_missing: if set to True, raise RuntimeError if the process does not already exist
    """
    if pid is None:
        logger.warning("Killing process id of 'None' will terminate the current python process!")
    logger.info(f"Killing processes with id '{pid}'")
    process = psutil.Process(pid)
    if process.is_running():
        _safe_kill_process(process)
    elif raise_on_missing:
        message = f"Process with id {pid} was not present"
        logger.error(message)
        raise RuntimeError(message)


def process_exists(name, ignore_extensions=False):
    """
    Determines whether a process with the given name exists

    :param name: process name
    :param ignore_extensions: ignore trailing file extension
    :return: A boolean determining whether the process is alive or not
    """
    name = name.lower()
    if name.isspace():
        return False

    if ignore_extensions:
        name_extensionless = _remove_extension(name)

    for process in _safe_get_processes(["name"]):
        try:
            proc_name = process.name().lower()
        except psutil.NoSuchProcess as e:
            logger.debug(f"Process '{process}' was active when list of processes was requested but it was not found "
                         f"during process_exists()", exc_info=True)
            continue
        except psutil.AccessDenied as e:
            logger.info(f"Permissions issue on {process} during process_exists check", exc_info=True)
            continue

        if proc_name == name:  # abc.exe matches abc.exe
            return True
        if ignore_extensions:
            proc_name_extensionless = _remove_extension(proc_name)
            if proc_name_extensionless == name:  # abc matches abc.exe
                return True
            if proc_name == name_extensionless:  # abc.exe matches abc
                return True
            # don't check proc_name_extensionless against name_extensionless: abc.exe and abc.exe are already tested,
            # however xyz.Gamelauncher should not match xyz.DedicatedServer
    return False


def process_is_unresponsive(name):
    """
    Check if the specified process is unresponsive.

    Mac warning: this method assumes that a process is not responsive if it is sleeping or waiting, this is true for
    'active' applications, but may not be the case for power optimized applications.

    :param name: the name of the process to check
    :return: True if the specified process is unresponsive and False otherwise
    """
    if ly_test_tools.WINDOWS:
        output = check_output(['tasklist',
                               '/FI', f'IMAGENAME eq {name}',
                               '/FI', 'STATUS eq NOT RESPONDING'])
        output = output.split(os.linesep)
        for line in output:
            if line and name.startswith(line.split()[0]):
                logger.debug(f"Process '{name}' was unresponsive.")
                logger.debug(line)
                return True
        logger.debug(f"Process '{name}' was not unresponsive.")
        return False
    else:
        cmd = ["ps", "-axc", "-o", "command,state"]
        output = check_output(cmd)
        for line in output.splitlines()[1:]:
            info = [l.strip() for l in line.split(" ") if l.strip() != '']
            state = info[-1]
            pname = " ".join(info[0:-1])

            if pname == name:
                logger.debug(f"{pname}: {state}")
                if "R" not in state:
                    logger.debug(f"Process {name} was unresponsive.")
                    return True
        logger.debug(f"Process '{name}' was not unresponsive.")
        return False


def check_output(command, **kwargs):
    """
    Forwards arguments to subprocess.check_output so better error messages can be displayed upon failure.

    If you need the stderr output from a failed process then pass in stderr=subprocess.STDOUT as a kwarg.

    :param command: A list of the command to execute and its arguments as split by whitespace.
    :param kwargs: Keyword args forwarded to subprocess.check_output.
    :return: Output from the command if it succeeds.
    """
    cmd_string = command
    if type(command) == list:
        cmd_string = ' '.join(command)

    logger.info(f'Executing "check_output({cmd_string})"')
    try:
        output = subprocess.check_output(command, **kwargs).decode(_PROCESS_OUTPUT_ENCODING)
    except subprocess.CalledProcessError as e:
        logger.error(f'Command "{cmd_string}" failed with returncode {e.returncode}, output:\n{e.output}')
        raise
    logger.info(f'Successfully executed "check_output({cmd_string})"')
    return output


def safe_check_output(command, **kwargs):
    """
    Forwards arguments to subprocess.check_output so better error messages can be displayed upon failure.
    This function eats the subprocess.CalledProcessError exception upon command failure and returns the output.

    If you need the stderr output from a failed process then pass in stderr=subprocess.STDOUT as a kwarg.

    :param command: A list of the command to execute and its arguments as split by whitespace.
    :param kwargs: Keyword args forwarded to subprocess.check_output.
    :return: Output from the command regardless of its return value.
    """
    cmd_string = command
    if type(command) == list:
        cmd_string = ' '.join(command)

    logger.info(f'Executing "check_output({cmd_string})"')
    try:
        output = subprocess.check_output(command, **kwargs).decode(_PROCESS_OUTPUT_ENCODING)
    except subprocess.CalledProcessError as e:
        output = e.output
        logger.warning(f'Command "{cmd_string}" failed with returncode {e.returncode}, output:\n{e.output}')
    else:
        logger.info(f'Successfully executed "check_output({cmd_string})"')
    return output


def check_call(command, **kwargs):
    """
    Forwards arguments to subprocess.check_call so better error messages can be displayed upon failure.

    :param command: A list of the command to execute and its arguments as if split by whitespace.
    :param kwargs: Keyword args forwarded to subprocess.check_call.
    :return: An exitcode of 0 if the call succeeds.
    """
    cmd_string = command
    if type(command) == list:
        cmd_string = ' '.join(command)

    logger.info(f'Executing "check_call({cmd_string})"')
    try:
        subprocess.check_call(command, **kwargs)
    except subprocess.CalledProcessError as e:
        logger.error(f'Command "{cmd_string}" failed with returncode {e.returncode}')
        raise
    logger.info(f'Successfully executed "check_call({cmd_string})"')
    return 0


def safe_check_call(command, **kwargs):
    """
    Forwards arguments to subprocess.check_call so better error messages can be displayed upon failure.
    This function eats the subprocess.CalledProcessError exception upon command failure and returns the exit code.

    :param command: A list of the command to execute and its arguments as if split by whitespace.
    :param kwargs: Keyword args forwarded to subprocess.check_call.
    :return: An exitcode of 0 if the call succeeds, otherwise the exitcode returned from the failed subprocess call.
    """
    cmd_string = command
    if type(command) == list:
        cmd_string = ' '.join(command)

    logger.info(f'Executing "check_call({cmd_string})"')
    try:
        subprocess.check_call(command, **kwargs)
    except subprocess.CalledProcessError as e:
        logger.warning(f'Command "{cmd_string}" failed with returncode {e.returncode}')
        return e.returncode
    else:
        logger.info(f'Successfully executed "check_call({cmd_string})"')
    return 0


def _safe_get_processes(attrs=None):
    """
    Returns the process iterator without raising an error if the process list changes

    :return: The process iterator
    """
    processes = None
    max_attempts = 10
    for _ in range(max_attempts):
        try:
            processes = psutil.process_iter(attrs)
            break
        except (psutil.Error, RuntimeError):
            logger.debug("Unexpected error", exc_info=True)
            continue
    return processes


def _safe_kill_process(proc):
    """
    Kills a given process without raising an error

    :param proc: The process to kill
    """
    try:
        logger.info(f"Terminating process '{proc.name()}' with id '{proc.pid}'")
        _terminate_and_confirm_dead(proc)
    except psutil.AccessDenied:
        logger.warning("Termination failed, Access Denied", exc_info=True)
    except psutil.NoSuchProcess:
        logger.debug("Termination request ignored, process was already terminated during iteration", exc_info=True)
    except Exception:  # purposefully broad
        logger.warning("Unexpected exception while terminating process", exc_info=True)


def _safe_kill_processes(processes):
    """
    Kills a given process without raising an error

    :param processes: An iterable of processes to kill
    """
    for proc in processes:
        try:
            logger.info(f"Terminating process '{proc.name()}' with id '{proc.pid}'")
            proc.kill()
        except psutil.AccessDenied:
            logger.warning("Termination failed, Access Denied with stacktrace:", exc_info=True)
        except psutil.NoSuchProcess:
            logger.debug("Termination request ignored, process was already terminated during iteration with stacktrace:", exc_info=True)
        except Exception:  # purposefully broad
            logger.debug("Unexpected exception ignored while terminating process, with stacktrace:", exc_info=True)

    def on_terminate(proc):
        logger.info(f"process '{proc.name()}' with id '{proc.pid}' terminated with exit code {proc.returncode}")
    try:
        psutil.wait_procs(processes, timeout=30, callback=on_terminate)
    except Exception:  # purposefully broad
        logger.debug("Unexpected exception while waiting for processes to terminate, with stacktrace:", exc_info=True)


def _terminate_and_confirm_dead(proc):
    """
    Kills a process and waits for the process to stop running.

    :param proc: A process to kill, and wait for proper termination
    """
    def killed():
        return not proc.is_running()

    proc.kill()
    waiter.wait_for(killed, exc=RuntimeError("Process did not terminate after kill command"))


def _remove_extension(filename):
    """
    Returns a file name without its extension, if any is present

    :param filename: The name of a file
    :return: The name of the file without the extension
    """
    return filename.rsplit(".", 1)[0]


def close_windows_process(pid, timeout=20, raise_on_missing=False):
    # type: (int, int, bool) -> None
    """
    Closes a window using the windows api and checks the return code. An error will be raised if the window hasn't
    closed after the timeout duration.
    Note: This is for Windows only and will fail on any other OS

    :param pid: The process id of the process window to close
    :param timeout: How long to wait for the window to close (seconds)
    :return: None
    :param pid: the pid of the process to kill
    :param raise_on_missing: if set to True, raise RuntimeError if the process does not already exist
    """
    if not ly_test_tools.WINDOWS:
        raise NotImplementedError("close_windows_process() is only implemented on Windows.")

    if pid is None:
        raise TypeError("Cannot close window with pid of None")

    if not psutil.Process(pid).is_running():
        if raise_on_missing:
            message = f"Process with id {pid} was unexpectedly not present"
            logger.error(message)
            raise RuntimeError(message)
        else:
            logger.warning(f"Process with id {pid} was not present but option raise_on_missing is disabled. Unless "
                           f"a matching process gets opened, calling close_windows_process will spin until its timeout")

    # Gain access to windows api
    user32 = ctypes.windll.user32
    # Set up C data types and function params
    WNDENUMPROC = ctypes.WINFUNCTYPE(ctypes.wintypes.BOOL,
                                     ctypes.wintypes.HWND,
                                     ctypes.wintypes.LPARAM)
    user32.EnumWindows.argtypes = [
        WNDENUMPROC,
        ctypes.wintypes.LPARAM]
    user32.GetWindowTextLengthW.argtypes = [
        ctypes.wintypes.HWND]

    # This is called for each process window
    def _close_matched_process_window(hwnd, _):
        # type: (ctypes.wintypes.HWND, int) -> bool
        """
        EnumWindows() takes a function argument that will return True/False to keep iterating or not.

        Checks the windows handle's pid against the given pid. If they match, then the window will be closed and
        returns False.

        :param hwnd: A windows handle to check against the pid
        :param _: Unused buffer parameter
        :return: False if process was found and closed, else True
        """
        # Get the process id of the handle
        lpdw_process_id = ctypes.c_ulong()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(lpdw_process_id))
        process_id = lpdw_process_id.value
        # Compare to the process id
        if pid == process_id:
            # Close the window
            WM_CLOSE = 16  # System message for closing window: 0x10
            user32.PostMessageA(hwnd, WM_CLOSE, 0, 0)
            # Found window for process id, stop iterating
            return False
        # Process not found, keep looping
        return True

    # Call the function on all of the handles
    close_process_func = WNDENUMPROC(_close_matched_process_window)
    user32.EnumWindows(close_process_func, 0)

    # Wait for asyncronous termination
    waiter.wait_for(lambda: pid not in psutil.pids(), timeout=timeout,
                    exc=TimeoutError(f"Process {pid} never terminated"))


def get_display_env():
    """
    Fetches environment variables with an appropriate display (monitor) configured,
      useful for subprocess calls to UI applications
    :return: A dictionary containing environment variables (per os.environ)
    """
    env = os.environ.copy()
    if not ly_test_tools.WINDOWS:
        if 'DISPLAY' not in env.keys():
            # assume Display 1 is available in another session
            env['DISPLAY'] = ':1'
    return env
