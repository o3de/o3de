"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Tools for inspecting GitHub CODEOWNERS files
"""

import argparse
import logging
import os
import pathlib
import re

logger = logging.getLogger(__name__)

_DEFAULT_CODEOWNER_ALIAS = "https://www.o3de.org/community/"
_GITHUB_CODEOWNERS_BYTE_LIMIT = 3 * 1024 * 1024  # 3MB


def get_codeowners(target_path: pathlib.PurePath) -> (str|None, str|None, pathlib.PurePath|None):
    """
    Finds ownership information matching the target filesystem path from a CODEOWNERS file found in its GitHub repo
    :param target_path: path to match in a GitHub CODEOWNERS file, which will be discovered inside its repo
    :return: tuple of (matched_path_entry, matched_owner_aliases, found_codeowners_path) which are empty when missing
    """
    codeowners_path = find_github_codeowners(target_path)
    if codeowners_path:
        matched_path, owner_aliases = get_codeowners_from(target_path, codeowners_path)
    else:
        matched_path = ""
        owner_aliases = ""
        codeowners_path = ""
    return matched_path, owner_aliases, codeowners_path


def find_github_codeowners(target_path: pathlib.PurePath) -> pathlib.Path|None:
    """
    Finds the '.github/CODEOWNERS' file for the git repo containing target_path, scanning upward through the filesystem
    :param target_path: a path expected to exist in a GitHub repository containing a CODEOWNERS file
    :return: path to the CODEOWNERS file, or None if no file could be located
    """
    current_path = target_path
    for _ in range(1000):
        codeowners_path = os.path.join(current_path, ".github", "CODEOWNERS")
        if os.path.exists(codeowners_path):
            return pathlib.Path(codeowners_path)
        next_path = os.path.dirname(current_path)
        if next_path == current_path:
            break  # reached filesystem root
        current_path = next_path

    logger.warning(f"No GitHub CODEOWNERS file found in a GitHub repo which contains {target_path}")
    return None


def get_codeowners_from(target_path: pathlib.PurePath, codeowners_path: pathlib.PurePath) -> (str, str):
    """
    Fetch ownership information matching the target filesystem path from a CODEOWNERS file
    :param target_path: path to match in the GitHub CODEOWNERS file
    :param codeowners_path: path to CODEOWNERS file
    :return: tuple of (matched_path_entry, matched_owner_aliases), which will be empty when nothing was matched.
        The aliases will also be empty when a matched path is explicitly unowned.
    """
    if not os.path.isfile(codeowners_path):
        logger.warning(f"No GitHub CODEOWNERS file found at {codeowners_path}")
        return "", ""

    if os.path.getsize(codeowners_path) > _GITHUB_CODEOWNERS_BYTE_LIMIT:
        logger.warning(f"GitHub CODEOWNERS file found at {codeowners_path} exceeds the standard limit of "
                       f"{_GITHUB_CODEOWNERS_BYTE_LIMIT} bytes")
        return "", ""

    # operate only on unix-style separators
    repo_root = pathlib.PurePosixPath(codeowners_path.parent.parent)
    unix_normalized_target = pathlib.PurePosixPath(target_path)
    if not unix_normalized_target.is_relative_to(repo_root):
        logger.warning(f"Path '{target_path}' is not inside the repo of GitHub CODEOWNERS file {codeowners_path}")
        return "", ""

    repo_relative_target = unix_normalized_target.relative_to(repo_root)
    repo_rooted_target = pathlib.PurePosixPath("/" + str(repo_relative_target))  # relative_to removes leading slash

    with open(codeowners_path) as codeowners_file:
        # GitHub syntax only applies the final matching rule ==> parse in reverse order and take first match
        for line in reversed(list(codeowners_file)):
            clean_line = line.strip()
            if clean_line and not clean_line.startswith('#'):  # ignore blanks and full-line comments
                # entry format should be "owned/path/     @alias1 @alias2 user@example.com @aliasN..."
                split_entry = line.split(maxsplit=1)
                owned_path = split_entry[0]
                if _codeowners_path_matches(repo_rooted_target, owned_path):
                    if len(split_entry) > 1:
                        aliases = split_entry[1].split("#", maxsplit=1)[0].strip()  # remove trailing comment
                    else:  # explicitly unowned entry with no comment
                        aliases = ""
                    return owned_path, aliases
                # else invalid entry syntax, ignore

    return "", ""  # no match found


def _codeowners_path_matches(target_path: pathlib.PurePosixPath, owned_path: str) -> bool:
    """
    :param target_path: PurePosixPath to match against, which starts from the root of the repo
    :param owned_path: path identifier found in a GitHub CODEOWNERS file (relative to root, may contain wildcards)
    :return: True when target_path is contained by the rules of owned_path
    """
    matched = False
    if '*' in owned_path or '?' in owned_path:  # wildcards require glob matching
        if owned_path.startswith("*"):  # special simple case for global wildcards
            matched = target_path.match(owned_path)
        elif owned_path.startswith("/"):  # ownership of specific directory: glob A against B
            matched = target_path.match(owned_path[1:])
        else:  # ownership of all relative directories: find non-wildcard portions of B in A, glob the remainders
            asterisk = owned_path.find("*")
            question = owned_path.find("?")
            if asterisk > -1 and question > -1:
                first_wildcard_index = min(asterisk, question)
            else:  # avoid not-found index
                first_wildcard_index = max(asterisk, question)
            separator_indices = [index.start() for index in re.finditer(pattern="/", string=owned_path)]
            pre_wildcard_separator_index = 0
            for s_index in separator_indices:
                if s_index < first_wildcard_index:
                    pre_wildcard_separator_index = s_index
                else:  # remainder are all greater
                    break

            # separate non-wildcard-containing path from remainder
            pre_wildcard_owned = owned_path[:pre_wildcard_separator_index]
            wildcard_with_remainder_owned = owned_path[pre_wildcard_separator_index+1:]

            # find substrings of initial portion of B within A
            target_str = str(target_path)
            pre_wildcard_target_end_indices = [index.end() for index in
                                               re.finditer(pattern=pre_wildcard_owned, string=target_str)]
            # glob remainders of A against remainder of B
            for target_index in pre_wildcard_target_end_indices:  # may be multiple substring matches within target
                target_remainder = target_str[target_index:]
                if pathlib.PurePosixPath(target_remainder).match(wildcard_with_remainder_owned):
                    matched = True
                    break  # exit early on success
    else:  # simple path matching
        if owned_path.startswith("/"):  # ownership of specific directory: verify if A exists inside B
            matched = target_path.is_relative_to(owned_path)
        else:  # ownership of all relative directories: verify if B is a substring of A
            matched = owned_path in str(target_path)
    return matched


def _pretty_print_success(print_fn, found_codeowners_path, matched_path, owner_aliases) -> None:
    """
    Prints a friendly message, instead of the default terse output of owner alias(es)
    :param print_fn: function to call when logging strings
    :param found_codeowners_path: verified path to a GitHub CODEOWNERS file
    :param matched_path: first part of an entry matched in the CODEOWNERS file
    :param owner_aliases: second part of an entry in a CODEOWNERS file
    """
    print_fn(f"Matched '{matched_path}' in file {found_codeowners_path}")
    print_fn(f"For additional support please reach out to: {owner_aliases}")


def _pretty_print_failure(print_fn, found_codeowners_path, matched_path, original_target,
                          default_alias=_DEFAULT_CODEOWNER_ALIAS) -> None:
    """
    Prints a friendly message about failure to find an owner
    :param print_fn: function to call when logging strings
    :param found_codeowners_path: verified path to a GitHub CODEOWNERS file which, empty when missing
    :param matched_path: entry matched in the CODEOWNERS file, empty when not matched
    :param original_target: the path which matching was attempted on
    :param default_alias: who to contact as no owner was found
    """
    if not found_codeowners_path:
        print_fn(f"No GitHub CODEOWNERS file was found for '{original_target}'")
    else:
        if not matched_path:
            print_fn(f"No ownership information for '{original_target}' found in file {found_codeowners_path}")
        else:
            print_fn(f"Ownership for '{matched_path}' is explicitly empty in file {found_codeowners_path}")
    print_fn(f"For additional support please reach out to: {default_alias}")


def _main() -> int:
    parser = argparse.ArgumentParser(description="Display GitHub CODEOWNERS information to stdout for a target path")
    parser.add_argument('target', metavar='T', type=pathlib.Path,
                        help="file path to find an owner for")
    parser.add_argument('-c', '--codeowners', type=pathlib.Path,
                        help="path to a GitHub CODEOWNERS file, when not set this will scan upward to find the repo "
                             "containing the target")
    parser.add_argument('-d', '--default_alias', default=_DEFAULT_CODEOWNER_ALIAS,
                        help="a default location to reach out for support, for when ownership cannot be determined")
    parser.add_argument('-p', '--pretty_print', action='store_true',
                        help="output ownership info as a friendly message instead of only alias(es)")
    parser.add_argument('-s', '--silent', action='store_true',
                        help="Suppress any warning messages and only print ownership information")
    args = parser.parse_args()

    if args.silent:
        logging.disable()
    else:
        logging.basicConfig()

    if args.codeowners:
        matched_path, owner_aliases = get_codeowners_from(args.target, args.codeowners)
        found_codeowners = os.path.isfile(args.codeowners)
    else:
        matched_path, owner_aliases, found_codeowners = get_codeowners(args.target)

    if owner_aliases and matched_path and found_codeowners:
        if args.pretty_print:
            _pretty_print_success(print, found_codeowners, matched_path, owner_aliases)
        else:
            print(owner_aliases)
        return 0
    else:
        if args.pretty_print:
            _pretty_print_failure(print, found_codeowners, matched_path, args.target, args.default_alias)
        else:
            print(args.default_alias)
        return 1

    logger.error("Unexpected abnormal exit")
    return -1
