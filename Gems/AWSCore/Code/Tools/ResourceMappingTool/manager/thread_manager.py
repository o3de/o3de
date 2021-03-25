"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from __future__ import annotations
import logging
from PySide2.QtCore import (QRunnable, QThreadPool)

from model import error_messages

logger = logging.getLogger(__name__)


class ThreadManager(object):
    """
    Thread manager maintains thread pool for executing/tracking thread worker
    """
    __instance: ThreadManager = None

    @staticmethod
    def get_instance() -> ThreadManager:
        if ThreadManager.__instance is None:
            ThreadManager()
        return ThreadManager.__instance

    def __init__(self) -> None:
        if ThreadManager.__instance is None:
            self._thread_pool: QThreadPool = QThreadPool()
            ThreadManager.__instance = self
        else:
            raise AssertionError(error_messages.SINGLETON_OBJECT_ERROR_MESSAGE.format("ThreadManager"))

    def setup(self, thread_count: int = 1) -> None:
        # Based on prototype use case, we just need 1 thread
        logger.info(f"Setting up thread pool with MaxThreadCount={thread_count} ...")
        self._thread_pool.setMaxThreadCount(thread_count)

    """Reserves a thread and uses it to run runnable worker, unless this thread will make
    the current thread count exceed max thread count. In that case, runnable is added to a run queue instead."""
    def start(self, worker: QRunnable) -> None:
        self._thread_pool.start(worker)

    """Removes the runnables that are not yet started from the queue. """
    def clear(self) -> None:
        self._thread_pool.clear()
