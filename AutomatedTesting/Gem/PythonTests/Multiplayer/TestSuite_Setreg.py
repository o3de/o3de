"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

import pytest
import os
import sys
from ly_test_tools.o3de.editor_test import EditorTestSuite

from .utils.FileManagement import FileManagement as fm

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

import ly_test_tools.environment.process_utils as       process_utils
import ly_test_tools.launchers.launcher_helper as       launcher_helper
from   ly_test_tools.log.log_monitor           import   LogMonitor
import ly_test_tools.environment.waiter        as       waiter

@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

	def _setup(self, workspace, target_file, src_file, parent_path=None, search_subdirs=False):
		"""
		Inner function to wrap around decorated function. Function being decorated must match this
		functions parameter order.
		"""
		self.target_file = target_file

		root_path = parent_path
		if root_path is not None:
			root_path = os.path.join(workspace.paths.engine_root(), root_path)
		else:
			# Default to project folder (AutomatedTesting)
			root_path = workspace.paths.project()

		# Try to locate both target and source files
		try:
			self.file_list = fm._find_files([target_file, src_file], root_path, search_subdirs)
		except RuntimeWarning as w:
			assert False, (
				w.args[0]
				+ " Please check use of search_subdirs; make sure you are using the correct parent directory."
			)

		fm._restore_file(target_file, self.file_list[target_file])
		fm._backup_file(target_file, self.file_list[target_file])
		fm._copy_file(src_file, self.file_list[src_file], target_file, self.file_list[target_file])
		# Run the test now.
		
	def _teardown(self):
		"""
		Cleanup
		"""
		fm._restore_file(self.target_file, self.file_list[self.target_file])

	def test_Multiplayer_SimpleGameServerLauncher_ConnectsSuccessfully_WithSetRegConfig(self, workspace, launcher_platform, crash_log_watchdog):
		"""
		"""
		try:
			self._setup(workspace, 'autoexec.server.setreg','autoexec.server_host.setreg_override','AutomatedTesting/Registry', search_subdirs=True)
			
			halt_on_unexpected = False
			timeout = 180
			server_unexpected_lines = []
			server_expected_lines = ["Executing console command 'host'"]

			workspace.asset_processor.start(connect_to_ap=True, connection_timeout=10)  # verify connection
			#workspace.asset_processor.wait_for_idle()

			# Start the AutomatedTesting.ServerLauncher.exe in hosting mode, no rendering mode, and wait for it to exist
			server_launcher = launcher_helper.create_server_launcher(workspace)
			server_launcher.args.extend(['-rhi=dx12','+host'])
			server_launcher.start()
			waiter.wait_for(lambda: process_utils.process_exists(f"AutomatedTesting.ServerLauncher.exe", ignore_extensions=True))

			# Start the AutomatedTesting.GameLauncher.exe in client mode, no rendering mode, and wait for it to exist
			# game_launcher = launcher_helper.create_game_launcher(workspace)
			# game_launcher.args.extend(['--rhi=dx12']) # '+connect' is in autoexec.client.setreg
			# game_launcher.start()
			# waiter.wait_for(lambda: process_utils.process_exists(f"AutomatedTesting.GameLauncher.exe", ignore_extensions=True))


			# Verify that the GameLauncher.exe was able to connect to the ServerLauncher.exe by checking the logs
			# client_unexpected_lines = []
			# client_expected_lines = ["Executing console command 'connect'"]

			# game_launcher_log_file = os.path.join(game_launcher.workspace.paths.project_log(), 'Game.log')
			# game_launcher_log_monitor = LogMonitor(game_launcher, game_launcher_log_file)
			# game_launcher_log_monitor.monitor_log_for_lines(client_expected_lines, client_unexpected_lines, halt_on_unexpected, timeout)

			# Verify that the ServerLauncher.exe was able to host by checking the logs
			server_launcher_log_file = os.path.join(server_launcher.workspace.paths.project_log(), 'Server.log')
			server_launcher_log_monitor = LogMonitor(server_launcher, server_launcher_log_file)
			server_launcher_log_monitor.monitor_log_for_lines(server_expected_lines, server_unexpected_lines, halt_on_unexpected, timeout)

			# Cleanup
			#game_launcher.stop()
			server_launcher.stop()

			workspace.asset_processor.stop(timeout=1)

		finally:
			self._teardown()
