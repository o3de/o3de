from o3de.export_project import *
from o3de import manifest
import argparse
import pathlib
import os
import shutil
#Note: project_path and engine_path are computed before this script runs, and can be accessed via o3de_context.project_path
#for now this script assumes an engine-centric approach, and we assume to use the source built engine for the engine_path

#to use this script, go to the engine root directory, and issue the following command:
#.\scripts\o3de.bat export-project -pp C:\path\to\project -es C:\path\to\export_profile_monolithic.py -nmp C:\path\to\build\non_mono -mp C:\path\to\build\mono -out C:\path\to\output -ll INFO

parser = argparse.ArgumentParser(prog="Profile Monolithic exporter for windows",
                                 description="Exports a profile monolithic packaged build for the project")
parser.add_argument('-nmp', '--non-mono-build-path', type=pathlib.Path, required=True)
parser.add_argument('-mp', '--mono-build-path',type=pathlib.Path, required=True)
parser.add_argument('-out', '--output-path', type=pathlib.Path, required=True)

args = parser.parse_args(o3de_context.args)

projects_path_cmake_option = f"-DLY_PROJECTS={str(o3de_context.project_path)}"

project_json_data = manifest.get_project_json_data(project_path = o3de_context.project_path)
project_name= project_json_data.get('project_name')

#clean the output folder if necessary
if os.path.isdir(args.output_path):
    shutil.rmtree(args.output_path)


#build non-monolithic artifacts
process_command(['cmake', '-S', '.', '-B', args.non_mono_build_path, '-DLY_MONOLITHIC_GAME=0', projects_path_cmake_option], cwd= o3de_context.engine_path)

process_command(['cmake', '--build', args.non_mono_build_path, '--target', 'Editor', '--target', 'AssetBundler', '--target', 'AssetProcessor', '--target', 'AssetProcessorBatch', '--config', 'profile'], cwd=o3de_context.engine_path)

executables_dir = args.non_mono_build_path / 'bin' / 'profile'

asset_processor_exe = executables_dir / 'AssetProcessorBatch.exe'

asset_bundler_exe = executables_dir / 'AssetBundler.exe'

process_command([asset_processor_exe, '--project-path', o3de_context.project_path], cwd=o3de_context.engine_path)


#build monolithic artifacts
process_command(['cmake', '-S', '.', '-B', args.mono_build_path, '-DLY_MONOLITHIC_GAME=1', '-DALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES=0', projects_path_cmake_option], cwd = o3de_context.engine_path)
process_command(['cmake', '--build',  args.mono_build_path, '--target', f'{project_name}.GameLauncher', '--target', f'{project_name}.ServerLauncher', '--target', f'{project_name}.UnifiedLauncher', '--config', 'profile'])

#bundle content (WIP)
# process_command([asset_bundler_exe, 'assetList','--assetListFile {}',f'--project-path="{str(o3de_context.project_path)}"'])

#copy artifacts to output directory

pc_pak_path = args.output_path / 'Cache' / 'pc'

os.makedirs(pc_pak_path, exist_ok=True)
os.makedirs(args.output_path, exist_ok=True)
# os.makedirs(args.output_path / 'Gems' / 'AWSCore')
# process_command(['copy', str(o3de_context.project_path / 'AssetBundling' / 'Bundles' / '*.pak'), str(pc_pak_path)])
source_path = str(args.mono_build_path / 'bin' / 'profile')
output_path = str(args.output_path)
process_command(f'xcopy {source_path} {output_path}', cwd=o3de_context.project_path)
