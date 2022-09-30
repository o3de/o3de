"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

File system related functions.
"""
import errno
import glob
import logging
import os
import psutil
import shutil
import stat
import sys
import tarfile
import time
import zipfile

import ly_test_tools.environment.process_utils as process_utils

logger = logging.getLogger(__name__)

ONE_KIB = 1024
ONE_MIB = 1024 * ONE_KIB
ONE_GIB = 1024 * ONE_MIB


def check_free_space(dest, required_space, msg):
    """ Make sure the required space is available on destination, raising an IOError if there is not. """
    free_space = psutil.disk_usage(dest).free
    if free_space < required_space:
        raise IOError(
            errno.ENOSPC,
            f'{msg} {free_space / ONE_GIB:.2f} GiB '
            f'vs {required_space / ONE_GIB:.2f} GiB')


def safe_makedirs(dest_path):
    """ This allows an OSError in the case the directory cannot be created, which is logged but does not propagate."""
    try:
        logger.info(f'Creating directory "{dest_path}"')
        os.makedirs(dest_path)

    except OSError as e:
        if e.errno == errno.EEXIST:
            pass
        elif e.errno == errno.EACCES and sys.platform == 'win32' and dest_path.endswith(':\\'):
            # In this case, windows will raise EACCES instead of EEXIST if you try to make a directory at the root.
            pass
        else:
            logger.info(f'Could not create directory: "{dest_path}".')
            raise


def get_newest_file_in_dir(path, exts):
    """ Find the newest file in a directory, matching the extensions provided. """
    dir_iter = []
    for ext in exts:
        dir_iter.extend(glob.iglob(os.path.join(path, ext)))

    try:
        return max(dir_iter, key=os.path.getctime)

    except ValueError:
        # May not be any files in that directory.
        return None


def remove_path_and_extension(src):
    """
    Given a src, will strip off the path and the extension. Used in unzip and untgz

    Example:
        C:\\packages\\lumberyard-XXXX.zip would become lumberyard-XXXX
    """
    src_name = os.path.basename(src)
    src_no_extension, _ = os.path.splitext(src_name)

    return src_no_extension


def set_up_decompression(full_size, dest, src, force, allow_exists=False):
    """
    Used in unzip and untgz, will check whether the dest has enough space and creates the new build path.

    :param full_size: Size of zipped package
    :param dest: Target unzip location
    :param src: Location of the zipped package
    :param force: Boolean determining whether to overwrite the build if it already exists
    :param allow_exists: Boolean determining whether to log critical if the build already exists
    :return: A tuple containing the unzipped build path and a bool determining whether the build already exists.
    """
    exists = False
    # Check free space leaving at least a GiB free.
    check_free_space(dest, full_size + ONE_GIB, 'Not enough space to safely extract: ')

    dst_path = os.path.join(dest, remove_path_and_extension(src))

    # Cannot easily compare the zip contents to existing dir. Assumes builds of the same name are identical.
    if os.path.exists(dst_path) and not force:
        exists = True
        # Only log critical if the user wants early termination of the command if the build exists
        if allow_exists:
            level = logging.getLevelName('INFO')
        else:
            level = logging.getLevelName('CRITICAL')
        logger.log(level, f'Found existing {dst_path}.  Will not overwrite.')
        return dst_path, exists

    return dst_path, exists


def unzip(dest, src, force=False, allow_exists=False):
    """
    decompress src_path\\name.zip to the dest directory in a subdirectory called name.
    Will strip assets names for lumberyard builds.

    Example:
        dest = D:\\builds
        src = C:\\packages\\lumberyard-XXXX.zip

    Result:
        C:\\packages\\lumberyard-XXXX.zip decompressed to D:\\builds\\lumberyard-XXXX

    src can be any file, but lumberyard asset builds will have their name shortened to match the build they belong to.
    """
    with zipfile.ZipFile(src, 'r') as zip_file:

        full_size = sum(info.file_size for info in zip_file.infolist())

        dst_path, exists = set_up_decompression(full_size, dest, src, force, allow_exists)

        if exists:
            return dst_path

        # Unzip and return final path.
        start_time = time.time()
        zip_file.extractall(dst_path)
        secs = time.time() - start_time
        if secs == 0:
            secs = 0.01

        logger.info(
            f'Extracted {full_size / ONE_GIB:.2f} GiB '
            f'from "{src}" to "{dst_path}" in '
            f'{secs / 60:2.2f} minutes, '
            f'at {(full_size / ONE_MIB) / secs:.2f} MiB/s.')

        return dst_path


def untgz(dest, src, exact_tgz_size=False, force=False, allow_exists=False):
    """
    decompress src_path\\name.tgz to the dest directory in a subdirectoy called name.
    Will strip assets names for lumberyard builds.

    Example:
        dest = D:\\builds
        src = C:\\packages\\lumberyard-XXXX.tgz

    Result:
        C:\\packages\\lumberyard-XXXX.tgz decompressed to D:\\builds\\lumberyard-XXXX

    src can be any file, but lumberyard asset builds will have their name shortened to match the build they belong to.
    """
    with tarfile.open(src) as tar_file:

        # Determine exact size of tar if instructed, otherwise estimate.
        if exact_tgz_size:
            full_size = 0
            for tarinfo in tar_file:
                full_size += tarinfo.size
        else:
            full_size = os.stat(src).st_size * 4.5

        dst_path, exists = set_up_decompression(full_size, dest, src, force, allow_exists)

        if exists:
            return dst_path

        # Extract it and return final path.
        start_time = time.time()
        def is_within_directory(directory, target):
            
            abs_directory = os.path.abspath(directory)
            abs_target = os.path.abspath(target)
        
            prefix = os.path.commonprefix([abs_directory, abs_target])
            
            return prefix == abs_directory
        
        def safe_extract(tar, path=".", members=None, *, numeric_owner=False):
        
            for member in tar.getmembers():
                member_path = os.path.join(path, member.name)
                if not is_within_directory(path, member_path):
                    raise Exception("Attempted Path Traversal in Tar File")
        
            tar.extractall(path, members, numeric_owner=numeric_owner)
            
        
        safe_extract(tar_file, dst_path)
        secs = time.time() - start_time
        if secs == 0:
            secs = 0.01

        logger.info(
            f'Extracted {full_size / ONE_GIB:.2f} MiB '
            f'from {src} to {dst_path} '
            f'in {secs / 60:2.2f} minutes, '
            f'at {(full_size / ONE_MIB) / secs:.2f} MiB/s.')

        return dst_path


def change_permissions(path_list, perms):
    """ Changes the permissions of the files and folders defined in the file list """
    try:
        for root, dirs, files in os.walk(path_list):
            for dir_name in dirs:
                os.chmod(os.path.join(root, dir_name), perms)
            for file_name in files:
                os.chmod(os.path.join(root, file_name), perms)
    except OSError as e:
        logger.warning(f"Couldn't change permission : Error: {e.filename} - {e.strerror}.")
        return False
    else:
        return True


def unlock_file(file_name):
    """
    Given a file name, unlocks the file for write access.

    :param file_name: Path to a file
    :return: True if unlock succeeded, else False
    """
    if not os.access(file_name, os.W_OK):
        file_stat = os.stat(file_name)
        os.chmod(file_name, file_stat.st_mode | stat.S_IWRITE)
        logger.info(f'Clearing write lock for file {file_name}.')
        return True
    else:
        logger.info(f'File {file_name} not write locked. Unlocking file not necessary.')
        return False


def lock_file(file_name):
    """
    Given a file name, lock write access to the file.

    :param file_name: Path to a file
    :return: True if lock succeeded, else False
    """
    if os.access(file_name, os.W_OK):
        file_stat = os.stat(file_name)
        os.chmod(file_name, file_stat.st_mode & (~stat.S_IWRITE))
        logger.info(f'Write locking file {file_name}')
        return True
    else:
        logger.info(f'File {file_name} already locked. Locking file not necessary.')
        return False


def remove_symlink(path):
    try:
        # Rmdir can delete a symlink without following the symlink to the original content
        os.rmdir(path)
    except OSError as e:
        if e.errno != errno.ENOTEMPTY:
            raise


def remove_symlinks(path, remove_root=False):
    """ Removes all symlinks at the provided path and its subdirectories. """
    for root, dirs, files in os.walk(path):
        for name in dirs:
            remove_symlink(os.path.join(root, name))
    if remove_root:
        remove_symlink(path)


def delete(file_list, del_files, del_dirs):
    """
    Given a list of directory paths, delete will remove all subdirectories and files based on which flag is set,
    del_files or del_dirs.

    :param file_list: A string or an array of artifact paths to delete
    :param del_files: True if delete should delete files
    :param del_dirs: True if delete should delete directories
    :return: True if delete was successful
    """
    if isinstance(file_list, str):
        file_list = [file_list]

    for file_to_delete in file_list:
        logger.info(f'Deleting "{file_to_delete}"')
        try:
            if del_dirs and os.path.isdir(file_to_delete):
                change_permissions(file_to_delete, 0o777)
                # Remove all symlinks before rmtree blows them away
                remove_symlinks(file_to_delete)
                shutil.rmtree(file_to_delete)
            elif del_files and os.path.isfile(file_to_delete):
                os.chmod(file_to_delete, 0o777)
                os.remove(file_to_delete)
        except OSError as e:
            logger.warning(f'Could not delete {e.filename} : Error: {e.strerror}.')
            return False
    return True


def create_backup(source, backup_dir, backup_name=None):
    """
    Creates a backup of a single source file by creating a copy of it with the same name + '.bak' in backup_dir
    e.g.: foo.txt is stored as backup_dir/foo.txt.bak
    If backup_name is provided, it will create a copy of the source file named "backup_name + .bak" instead.

    :param source: Full path to file to backup
    :param backup_dir: Path to the directory to store backup.
    :param backup_name: [Optional] Name of the backed up file to use instead or the source name.
    """

    if not backup_dir or not os.path.isdir(backup_dir):
        logger.error(f'Cannot create backup due to invalid backup directory {backup_dir}')
        return False

    if not os.path.exists(source):
        logger.warning(f'Source file {source} does not exist, aborting backup creation.')
        return False

    dest = None
    if backup_name is None:
        source_filename = os.path.basename(source)
        dest = os.path.join(backup_dir, f'{source_filename}.bak')
    else:
        dest = os.path.join(backup_dir, f'{backup_name}.bak')

    logger.info(f'Saving backup of {source} in {dest}')
    if os.path.exists(dest):
        logger.warning(f'Backup file already exists at {dest}, it will be overwritten.')

    try:
        shutil.copy2(source, dest)
    except Exception:  # intentionally broad
        logger.warning('Could not create backup, exception occurred while copying.', exc_info=True)
        return False

    return True

def restore_backup(original_file, backup_dir, backup_name=None):
    """
    Restores a backup file to its original location. Works with a single file only.

    :param original_file: Full path to file to overwrite.
    :param backup_dir: Path to the directory storing the backup.
    :param backup_name: [Optional] Provide if the backup file name is different from source. eg backup file = myFile_1.txt.bak original file = myfile.txt
    """

    if not backup_dir or not os.path.isdir(backup_dir):
        logger.error(f'Cannot restore backup due to invalid or nonexistent directory {backup_dir}.')
        return False

    backup = None
    if backup_name is None:
        source_filename = os.path.basename(original_file)
        backup = os.path.join(backup_dir, f'{source_filename}.bak')
    else:
        backup = os.path.join(backup_dir, f'{backup_name}.bak')

    if not os.path.exists(backup):
        logger.warning(f'Backup file {backup} does not exist, aborting backup restoration.')
        return False

    logger.info(f'Restoring backup of {original_file} from {backup}')
    try:
        shutil.copy2(backup, original_file)
    except Exception:  # intentionally broad
        logger.warning('Could not restore backup, exception occurred while copying.', exc_info=True)
        return False
    return True


def delete_oldest(path_glob, keep_num, del_files=True, del_dirs=False):
    """ Delete oldest builds, keeping a specific number """
    logger.info(
        f'Deleting dirs: {del_dirs} files: {del_files} "{path_glob}", keeping {keep_num}')
    paths = glob.iglob(path_glob)
    paths = sorted(paths, key=lambda fi: os.path.getctime(fi), reverse=True)
    return delete(paths[keep_num:], del_files, del_dirs)


def make_junction(dst, src):
    """Create a directory junction on Windows or a hardlink on macOS."""
    if not os.path.isdir(src):
        raise IOError(f"{src} is not a directory")
    elif sys.platform == 'win32':
        process_utils.check_output(["mklink", "/J", dst, src], shell=True)
    elif sys.platform == 'darwin':
        process_utils.check_output(["ln", dst, src])
    else:
        raise IOError(f"Unsupported operating system: {sys.platform}")


def split_path_where_exists(path):
    """
    Splits a path into 2 parts: the part that exists and the part that doesn't.

    :param path: the path to split
    :return: a tuple (exists_part, remainder) where exists_part is the part that exists and remainder is the part that 
             doesn't. Either part may be None.
    """
    current = path
    remainder = None
    while True:
        if os.path.exists(current):
            return current, remainder
        next_, tail = os.path.split(current)
        tail = tail or next_
        remainder = tail if remainder is None else os.path.join(tail, remainder)
        if next_ == current:
            break
        current = next_
    return None, remainder


def sanitize_file_name(file_name):
    """
    Replaces unsupported file name characters with a double underscore

    :param file_name: The target file name to sanitize
    :return: The sanitized name
    """
    return ''.join(
        '__' if c in ['\\', '/', ' ', ':', '*', '<', '>', '"', '|', '?'] + [chr(i) for i in range(32)] else c for c
        in file_name)


def reduce_file_name_length(file_name, max_length):
    """
    Reduces the length of the string file_name to match the length parameter.

    :param file_name: string for the file name to reduce in length.
    :param max_length: the length to reduce file_name to.
    :return: file name string with a maximum length matching max_length.
    """
    reduce_amount = len(file_name) - max_length

    if len(file_name) > max_length:
        file_name = file_name[:-reduce_amount]

    return file_name


def find_ancestor_file(target_file_name, start_path=os.getcwd()):
    """
    Find a file with the given name in the ancestor directories by walking up the starting path until the file is found.

    :param target_file_name: Name of the file to find.
    :param start_path: Optional path to start looking for the file.
    :return: Path to the file or None if not found.
    """
    current_path = os.path.abspath(start_path)
    candidate_path = os.path.join(current_path, target_file_name)

    # Limit the number of directories to traverse, to avoid infinite loop in path cycles
    for _ in range(15):
        if not os.path.exists(candidate_path):
            parent_path = os.path.dirname(current_path)
            if parent_path == current_path:
                # Only true when we are at the directory root, can't keep searching
                break
            candidate_path = os.path.join(parent_path, target_file_name)
            current_path = parent_path
        else:
            # Found the file we wanted
            break

    if not os.path.exists(candidate_path):
        logger.warning(f'The candidate path {candidate_path} does not exist.')
        return None

    return candidate_path
