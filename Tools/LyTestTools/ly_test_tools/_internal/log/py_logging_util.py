"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Manages logging.
"""
import contextlib
import sys
import logging

_stream_handler = None
_info_file_handler = None
_debug_file_handler = None


def initialize_logging(info_log_path, debug_log_path):
    """
    Stub method to centralize logging initialization, redirects everything to STDOUT.
    """
    log = logging.getLogger('')
    log.setLevel(logging.DEBUG)

    # %(asctime)s
    formatter = logging.Formatter("%(relativeCreated)s - %(levelname)s - [%(threadName)s] - %(name)s - %(message)s")

    # stdout
    global _stream_handler
    if _stream_handler is None:
        _stream_handler = logging.StreamHandler(stream=sys.stdout)
        _stream_handler.setLevel(logging.INFO)
        _stream_handler.setFormatter(formatter)
        log.addHandler(_stream_handler)

    global _info_file_handler
    if _info_file_handler is None:
        _info_file_handler = logging.FileHandler(info_log_path)
        _info_file_handler.setLevel(logging.INFO)
        _info_file_handler.setFormatter(formatter)
        log.addHandler(_info_file_handler)

    global _debug_file_handler
    if _debug_file_handler is None:
        _debug_file_handler = logging.FileHandler(debug_log_path)
        _debug_file_handler.setLevel(logging.DEBUG)
        _debug_file_handler.setFormatter(formatter)
        log.addHandler(_debug_file_handler)


def terminate_logging():
    """
    Removes all of the centralized logging handlers that were previously initialized.
    """
    log = logging.getLogger('')

    global _stream_handler
    if _stream_handler is not None:
        log.removeHandler(_stream_handler)
        _stream_handler = None

    global _info_file_handler
    if _info_file_handler is not None:
        log.removeHandler(_info_file_handler)
        _info_file_handler = None

    global _debug_file_handler
    if _debug_file_handler is not None:
        log.removeHandler(_debug_file_handler)
        _debug_file_handler = None


@contextlib.contextmanager
def suppress_logging():
    """ Use 'with suppress_logging():' to temporarily disable logging."""
    logging.disable(logging.CRITICAL)
    try:
        yield
    finally:
        logging.disable(logging.NOTSET)
