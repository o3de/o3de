#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#helper to generate settings file
def create_dummy_commands_settings_file(build_config='profile', tool_config='profile', archive_format='none',
                                        build_assets=False, fail_asset_errors=False, build_tools=False,
                                        tools_path='build/tools', launcher_path='build/launcher', android_path='build/game_android',
                                        build_ios='build/game_ios', allow_reg_overrides=False,
                                        asset_bundle_path='build/asset_bundling', max_size=2048,
                                        build_game_launcher=True, build_server_launcher=True, build_headless_server_launcher=True, build_unified_launcher=True,
                                        engine_centric = False, monolithic=False):
    return  f"""
[export_project]
project.build.config = {build_config}
tool.build.config = {tool_config}
archive.output.format = {archive_format}
option.build.assets = {str(build_assets)}
option.fail.on.asset.errors = {str(fail_asset_errors)}
seedlist.paths = 
seedfile.paths = 
default.level.names = 
additional.game.project.file.pattern.to.copy = 
additional.server.project.file.pattern.to.copy = 
additional.project.file.pattern.to.copy = 
option.build.tools = {str(build_tools)}
default.build.tools.path = {tools_path}
default.launcher.build.path = {launcher_path}
default.android.build.path = {android_path}
default.ios.build.path = {build_ios}
option.allow.registry.overrides = {str(allow_reg_overrides)}
asset.bundling.path = {asset_bundle_path}
max.size = {max_size}
option.build.game.launcher = {str(build_game_launcher)}
option.build.server.launcher = {str(build_server_launcher)}
option.build.headless.server.launcher = {str(build_headless_server_launcher)}
option.build.unified.launcher = {str(build_unified_launcher)}
option.engine.centric = {str(engine_centric)}
option.build.monolithic = {str(monolithic)}

[android]
platform.sdk.api = 30
ndk.version = 25*
android.gradle.plugin = 8.1.0
gradle.jvmargs = 
sdk.root = C:\\Users\\o3deuser\\AppData\\Local\\Android\\Sdk
signconfig.store.file = C:\\Users\\o3deuser\\O3DE\\Projects\\DevTestProject\\o3de-android-key.keystore
signconfig.key.alias = o3dekey
signconfig.store.password = o3depass
signconfig.key.password = o3depass
asset.mode = PAK
"""

def setup_local_export_config_test(tmpdir, **command_settings_kwargs):
    tmpdir.ensure('.o3de', dir=True)

    test_project_name = "TestProject"
    test_project_path = tmpdir/ 'O3DE' / "project"
    test_engine_path = tmpdir/ 'O3DE' / "engine"

    dummy_project_file = test_project_path / 'project.json'
    dummy_project_file.ensure()
    with dummy_project_file.open('w') as dpf:
        dpf.write("""
{
    "project_name": "TestProject",
    "project_id": "{11111111-1111-AAAA-AA11-111111111111}"
}
""")

    dummy_commands_file = tmpdir / '.o3de' / '.command_settings'
    with dummy_commands_file.open('w') as dcf:
        dcf.write(create_dummy_commands_settings_file(**command_settings_kwargs))
    
    return test_project_name, test_project_path, test_engine_path
