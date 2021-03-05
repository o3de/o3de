#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import sys
import glob_to_regex
import zipfile
import timeit
import stat
from optparse import OptionParser
from PackageEnv import PackageEnv
from ci_build import build
from utils.util import *
from utils.lib.glob3 import glob


def package(options):
    package_platform = options.package_platform
    package_env = PackageEnv(package_platform, options.package_env)
    engine_root = package_env.get('ENGINE_ROOT')

    # Ask the validator code to tell us which files need to be removed from the package
    prohibited_file_mask = get_prohibited_file_mask(package_platform, engine_root)

    # Scrub files. This is destructive, but is necessary to allow the current file existance checks to work properly. Better to copy and then build, or to
    # mask on sync, but this is what we have for now
    # No need to run scrubbing script since all restricted platform codes are moved to dev/restricted folder
    #scrub_files(package_env, prohibited_file_mask)

    # validate files
    validate_restricted_files(package_platform, package_env)

    # Override values in bootstrap.cfg for PC package
    override_bootstrap_cfg(package_env)

    # Generate GameTemplates whitelist information for metrics reporting
    template_whitelist_script = os.path.join(engine_root, 'Tools/build/JenkinsScripts/distribution/Metrics/GameTemplates/buildGameTemplateWhitelist.py')
    if os.path.exists(template_whitelist_script):
        if sys.platform == 'win32':
            python = os.path.join(engine_root, 'Tools', 'Python', 'python.cmd')
        else:
            python = os.path.join(engine_root, 'Tools', 'Python', 'python.sh')
        project_templates_folder = os.path.join(engine_root, 'ProjectTemplates')
        args = [python, template_whitelist_script, '--projectTemplatesFolder', project_templates_folder]
        #execute_system_call(args)

    if not package_env.get('SKIP_BUILD'):
        print('SKIP_BUILD is False, running CMake build...')
        cmake_build(package_env)

    # TODO Compile Assets
    #if package_env.exists('ASSET_PROCESSOR_PATH'):
    #    compile_assets(package_env)

    #create packages
    create_packages(package_env)


def override_bootstrap_cfg(package_env):
    print('Override values in bootstrap.cfg')
    engine_root = package_env.get('ENGINE_ROOT')
    bootstrap_path = os.path.join(engine_root, 'bootstrap.cfg')
    replace_values = {'sys_game_folder':'{}'.format(package_env.get('BOOTSTRAP_CFG_GAME_FOLDER'))}
    try:
        with open(bootstrap_path, 'r') as bootstrap_cfg:
            content = bootstrap_cfg.read()
    except:
        error('Cannot read file {}'.format(bootstrap_path))
    content = content.split('\n')
    new_content = []
    for line in content:
        if not line.startswith('--'):
            strs = line.split('=')
            if len(strs):
                key = strs[0].strip(' ')
                if key in replace_values:
                    line = '{}={}'.format(key, replace_values[key])
        new_content.append(line)
    try:
        with open(bootstrap_path, 'w') as out:
            out.write('\n'.join(new_content))
    except:
        error('Cannot write to file {}'.format(bootstrap_path))
    print('{} updated with value {}'.format(bootstrap_path, replace_values))


def get_prohibited_file_mask(package_platform, engine_root):
    sys.path.append(os.path.join(engine_root, 'Tools', 'build', 'JenkinsScripts', 'distribution', 'scrubbing'))
    from validator_data_LEGAL_REVIEW_REQUIRED import get_prohibited_platforms_for_package

    # The list of prohibited platforms is controlled by the validator on a per-package basis
    prohibited_platforms = get_prohibited_platforms_for_package(package_platform)
    prohibited_platforms.append('all')
    excludes_list = []
    for p in prohibited_platforms:
        platform_excludes = glob_to_regex.generate_excludes_for_platform(engine_root, p)
        excludes_list.extend(platform_excludes)
    prohibited_file_mask = re.compile('|'.join(excludes_list), re.IGNORECASE)
    return prohibited_file_mask


def scrub_files(package_env, prohibited_file_mask):
    print('Perform the Code Scrubbing')
    engine_root = package_env.get('ENGINE_ROOT')

    success = True
    for dirname, subFolders, files in os.walk(engine_root):
        for filename in files:
            full_path = os.path.join(dirname, filename)
            if prohibited_file_mask.match(full_path):
                try:
                    print('Deleting: {}'.format(full_path))
                    os.chmod(full_path, stat.S_IWRITE)
                    os.unlink(full_path)
                except:
                    e = sys.exc_info()[0]
                    sys.stderr.write('Error: could not delete {} ... aborting.\n'.format(full_path))
                    sys.stderr.write('{}\n'.format(str(e)))
                    success = False
    if not success:
        sys.stderr.write('ERROR: scrub_files failed\n')
        sys.exit(1)


def validate_restricted_files(package, package_env):
    print('Perform the Code Scrubbing')
    engine_root = package_env.get('ENGINE_ROOT')

    # Run validator
    success = True
    validator_path = os.path.join(engine_root, 'Tools/build/JenkinsScripts/distribution/scrubbing/validator.py')
    if sys.platform == 'win32':
        python = os.path.join(engine_root, 'Tools', 'Python', 'python3.cmd')
    else:
        python = os.path.join(engine_root, 'Tools', 'Python', 'python3.sh')
    args = [python, validator_path, '--package', package, engine_root]
    return_code = safe_execute_system_call(args)
    if return_code != 0:
        success = False
    if not success:
        error('Restricted file validator failed.')
    print('Restricted file validator completed successfully.')


def cmake_build(package_env):
    build_targets = package_env.get('BUILD_TARGETS')
    for build_target in build_targets:
        build(build_target['BUILD_CONFIG_FILENAME'], build_target['PLATFORM'], build_target['TYPE'])


def create_packages(package_env):
    package_targets = package_env.get('PACKAGE_TARGETS')
    for package_target in package_targets:
        print('Creating zipfile for package target {}'.format(package_target))
        cur_dir = os.path.dirname(os.path.abspath(__file__))
        filelist = os.path.join(cur_dir, 'package_filelists', '{}.json'.format(package_target['TYPE']))
        with open(filelist, 'r') as source:
            data = json.load(source)
        lyengine = os.path.dirname(package_env.get('ENGINE_ROOT'))
        print('Calculating filelists...')
        files = {}
        # We have to include 3rdParty in Mac/Xenia/Provo/Salem/Consoles package until LAD is available for those platforms
        # Remove this when LAD is available for those platforms.
        if package_target['TYPE'] in ['cmake_consoles', 'consoles']:
            files.update(get_3rdparty_filelist(package_env, 'common'))
            files.update(get_3rdparty_filelist(package_env, 'vc141'))
            files.update(get_3rdparty_filelist(package_env, 'vc142'))
            files.update(get_3rdparty_filelist(package_env, 'provo'))
            files.update(get_3rdparty_filelist(package_env, 'xenia'))
        elif package_target['TYPE'] in ['cmake_atom_pc']:
            files.update(get_3rdparty_filelist(package_env, 'common'))
            files.update(get_3rdparty_filelist(package_env, 'vc141'))
            files.update(get_3rdparty_filelist(package_env, 'vc142'))
        elif package_target['TYPE'] in ['cmake_all']:
            if package_env.get_target_platform() == 'mac':
                files.update(get_3rdparty_filelist(package_env, 'common'))
                files.update(get_3rdparty_filelist(package_env, 'mac'))
            elif package_env.get_target_platform() == 'consoles':
                files.update(get_3rdparty_filelist(package_env, 'common'))
                files.update(get_3rdparty_filelist(package_env, 'vc141'))
                files.update(get_3rdparty_filelist(package_env, 'vc142'))
                files.update(get_3rdparty_filelist(package_env, 'provo'))
                files.update(get_3rdparty_filelist(package_env, 'xenia'))

        if '@lyengine' in data:
            if '@engine_root' in data['@lyengine']:
                engine_root_basename = os.path.basename(package_env.get('ENGINE_ROOT'))
                data['@lyengine'][engine_root_basename] = data['@lyengine']['@engine_root']
                data['@lyengine'].pop('@engine_root')
            files.update(filter_files(data['@lyengine'], lyengine))
        if '@3rdParty' in data:
            files.update(filter_files(data['@3rdParty'], package_env.get('THIRDPARTY_HOME')))
        package_path = os.path.join(lyengine, package_target['PACKAGE_NAME'])
        print('Creating zipfile at {}'.format(package_path))
        start = timeit.default_timer()

        with zipfile.ZipFile(package_path, 'w', compression=zipfile.ZIP_DEFLATED, allowZip64=True) as myzip:
            for f in files:
                if os.path.islink(f):
                    zipInfo = zipfile.ZipInfo(files[f])
                    zipInfo.create_system = 3
                    # long type of hex val of '0xA1ED0000L',
                    # say, symlink attr magic...
                    #zipInfo.external_attr = 0xA1ED0000L
                    zipInfo.external_attr |= 0xA0000000
                    myzip.writestr(zipInfo, os.readlink(f))
                else:
                    myzip.write(f, files[f])

        stop = timeit.default_timer()
        total_time = int(stop - start)
        print('{} is created. Total time: {} seconds.'.format(package_path, total_time))

        def get_MD5(file_path):
            from hashlib import md5
            chunk_size = 200 * 1024
            h = md5()
            with open(file_path, 'rb') as f:
                while True:
                    chunk = f.read(chunk_size)
                    if len(chunk):
                        h.update(chunk)
                    else:
                        break
            return h.hexdigest()

        md5_file = '{}.MD5'.format(package_path)
        print('Creating MD5 file at {}'.format(md5_file))
        start = timeit.default_timer()
        with open(md5_file, 'w') as output:
            output.write(get_MD5(package_path))
        stop = timeit.default_timer()
        total_time = int(stop - start)
        print('{} is created. Total time: {} seconds.'.format(md5_file, total_time))


def filter_files(data, base, prefix='', support_symlinks=True):
    includes = {}
    excludes = set()
    for key, value in data.items():
        pattern = os.path.join(base, prefix, key)
        if not isinstance(value, dict):
            pattern = os.path.normpath(pattern)
            result = glob(pattern, recursive=True)
            files = [x for x in result if os.path.isfile(x) or (support_symlinks and os.path.islink(x))]
            if value == "#exclude":
                excludes.update(files)
            elif value == "#include":
                for file in files:
                    includes[file] = os.path.relpath(file, base)
            else:
                if value.startswith('#move:'):
                    for file in files:
                        file_name = os.path.relpath(file, os.path.join(base, prefix))
                        dst_dir = value.replace('#move:', '').strip(' ')
                        includes[file] = os.path.join(dst_dir, file_name)
                elif value.startswith('#rename:'):
                    for file in files:
                        dst_file = value.replace('#rename:', '').strip(' ')
                        includes[file] = dst_file
                else:
                    warn('Unknown directive {} for pattern {}'.format(value, pattern))
        else:
            includes.update(filter_files(value, base, os.path.join(prefix, key), support_symlinks))

    for exclude in excludes:
        try:
            includes.pop(exclude)
        except KeyError:
            pass
    return includes


def get_3rdparty_filelist(package_env, platform, support_symlinks=True):
    engine_root = package_env.get('ENGINE_ROOT')
    include_pattern_file = 'include_pattern_file'
    if os.path.isfile(include_pattern_file):
        os.remove(include_pattern_file)
    exclude_pattern_file = 'exclude_pattern_file'
    if os.path.isfile(exclude_pattern_file):
        os.remove(exclude_pattern_file)
    versions_file = 'versions_file'
    if os.path.isfile(versions_file):
        os.remove(versions_file)

    # Generate 3rdParty version file
    ly_dep_version_tool = os.path.join(engine_root, 'Tools/build/JenkinsScripts/distribution/ly_dep_version_tool.py')
    setup_assistant_config = os.path.join(engine_root, 'SetupAssistantConfig.json')
    if sys.platform == 'win32':
        python = os.path.join(engine_root, 'Tools', 'Python', 'python.cmd')
    else:
        python = os.path.join(engine_root, 'Tools', 'Python', 'python.sh')
    args = [python, ly_dep_version_tool, '-o', versions_file, '-s', setup_assistant_config]
    execute_system_call(args)

    # Generate 3rdParty include pattern and exclude pattern
    generate_external_3rdparty_file_list = os.path.join(engine_root, 'Tools/build/JenkinsScripts/distribution/ThirdParty/generate_external_3rdparty_file_list.py')
    package_config = os.path.join(engine_root, 'Tools/build/JenkinsScripts/distribution/ThirdParty/CMakePackageConfig.json')
    args = [python, generate_external_3rdparty_file_list, '-s', versions_file, '-c', package_config, '-p', platform, '-i', include_pattern_file, '-e', exclude_pattern_file]
    execute_system_call(args)

    # Calculate filelist using include pattern and exclude pattern
    thirdparty_home = package_env.get('THIRDPARTY_HOME')
    filelist = {}
    with open(include_pattern_file, 'r') as source:
        include_patterns = source.readlines()
    for include_pattern in include_patterns:
        pattern = os.path.join(thirdparty_home, include_pattern.strip('\n'))
        pattern = os.path.normpath(pattern)
        result = glob(pattern, recursive=True)
        files = [x for x in result if os.path.isfile(x) or (support_symlinks and os.path.islink(x))]
        for file in files:
            filelist[file] = os.path.join('3rdParty', os.path.relpath(file, thirdparty_home))

    with open(exclude_pattern_file, 'r') as source:
        exclude_patterns = source.readlines()
    for exclude_pattern in exclude_patterns:
        pattern = os.path.join(thirdparty_home, exclude_pattern.strip('\n'))
        pattern = os.path.normpath(pattern)
        result = glob(pattern, recursive=True)
        files = [x for x in result if os.path.isfile(x) or (support_symlinks and os.path.islink(x))]
        for file in files:
            try:
                filelist.pop(file)
            except KeyError:
                pass
    return filelist


def parse_args():
    cur_dir = os.path.dirname(os.path.abspath(__file__))
    parser = OptionParser()
    parser.add_option("--release", dest="release", default=False, action='store_true', help="Release build")
    parser.add_option("--package_platform", dest="package_platform", default='consoles', help="Target platform to package")
    parser.add_option("--package_env", dest="package_env", default=os.path.join(cur_dir, "cmake_package_env.json"),
                      help="JSON file that defines package environment variables")
    parser.add_option("--package_build_configurations_json", dest="package_build_configurations_json",
                      default=os.path.join(cur_dir, "package_build_configurations.json"),
                      help="JSON file that defines build parameters")
    (options, args) = parser.parse_args()

    if options.package_platform is None:
        error('No package platform specified')
    return options, args


if __name__ == "__main__":
    (options, args) = parse_args()
    package(options)




