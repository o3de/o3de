#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import os
import sys
import zipfile
import timeit
import progressbar
from optparse import OptionParser
from PackageEnv import PackageEnv
cur_dir = cur_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, f'{cur_dir}/..')
from ci_build import build
from util import *
from glob3 import glob


def package(options):
    package_env = PackageEnv(options.platform, options.type, options.package_env)

    if not package_env.get('SKIP_BUILD'):
        print(package_env.get('SKIP_BUILD'))
        print('SKIP_BUILD is False, running CMake build...')
        cmake_build(package_env)

    # TODO Compile Assets
    #if package_env.exists('ASSET_PROCESSOR_PATH'):
    #    compile_assets(package_env)

    #create packages
    package_targets = package_env.get('PACKAGE_TARGETS')
    for package_target in package_targets:
        create_package(package_env, package_target)
        upload_package(package_env, package_target)


def get_python_path(package_env):
    if sys.platform == 'win32':
        return os.path.join(package_env.get('ENGINE_ROOT'), 'python', 'python.cmd')
    else:
        return os.path.join(package_env.get('ENGINE_ROOT'), 'python', 'python.sh')


def cmake_build(package_env):
    build_targets = package_env.get('BUILD_TARGETS')
    for build_target in build_targets:
        build(build_target['BUILD_CONFIG_FILENAME'], build_target['PLATFORM'], build_target['TYPE'])


def create_package(package_env, package_target):
    print('Creating zipfile for package target {}'.format(package_target))
    cur_dir = os.path.dirname(os.path.abspath(__file__))
    file_list_type = package_target['FILE_LIST_TYPE']
    if file_list_type == 'All':
        filelist = os.path.join(cur_dir, 'package_filelists', package_target['FILE_LIST'])
    else:
        filelist = os.path.join(cur_dir, 'Platform', file_list_type, 'package_filelists', package_target['FILE_LIST'])
    with open(filelist, 'r') as source:
        data = json.load(source)
    lyengine = package_env.get('ENGINE_ROOT')
    print('Calculating filelists...')
    files = {}

    if '@lyengine' in data:
        files.update(filter_files(data['@lyengine'], lyengine))
    if '@3rdParty' in data:
        files.update(filter_files(data['@3rdParty'], package_env.get('THIRDPARTY_HOME')))
    package_path = os.path.join(lyengine, package_target['PACKAGE_NAME'])
    print('Creating zipfile at {}'.format(package_path))
    start = timeit.default_timer()
    with progressbar.ProgressBar(max_value=len(files), redirect_stderr=True) as bar:
        with zipfile.ZipFile(package_path, 'w', compression=zipfile.ZIP_DEFLATED, allowZip64=True) as myzip:
            i = 0
            bar.update(i)
            last_bar_update = timeit.default_timer()
            for f in files:
                if os.path.islink(f):
                    zipInfo = zipfile.ZipInfo(files[f])
                    zipInfo.create_system = 3
                    # long type of hex val of '0xA1ED0000L',
                    # say, symlink attr magic...
                    zipInfo.external_attr |= 0xA0000000
                    myzip.writestr(zipInfo, os.readlink(f))
                else:
                    myzip.write(f, files[f])
                i += 1
                # Update progress bar every 2 minutes
                if int(timeit.default_timer() - last_bar_update) > 120:
                    last_bar_update = timeit.default_timer()
                    bar.update(i)
            bar.update(i)

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


def upload_package(package_env, package_target):
    package_name = package_target['PACKAGE_NAME']
    engine_root = package_env.get('ENGINE_ROOT')
    internal_s3_bucket = package_env.get('INTERNAL_S3_BUCKET')
    qa_s3_bucket = package_env.get('QA_S3_BUCKET')
    s3_prefix = package_env.get('S3_PREFIX')
    print(f'Uploading {package_name} to S3://{internal_s3_bucket}/{s3_prefix}/{package_name}')
    cmd = ['aws', 's3', 'cp', os.path.join(engine_root, package_name), f's3://{internal_s3_bucket}/{s3_prefix}/{package_name}']
    execute_system_call(cmd, stdout=subprocess.DEVNULL)
    print(f'Uploading {package_name} to S3://{qa_s3_bucket}/{s3_prefix}/{package_name}')
    cmd = ['aws', 's3', 'cp', os.path.join(engine_root, package_name), f's3://{qa_s3_bucket}/{s3_prefix}/{package_name}', '--acl', 'bucket-owner-full-control']
    execute_system_call(cmd, stdout=subprocess.DEVNULL)


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


def parse_args():
    parser = OptionParser()
    parser.add_option("--platform", dest="platform", default='consoles', help="Target platform to package")
    parser.add_option("--type", dest="type", default='consoles', help="Package type")
    parser.add_option("--package_env", dest="package_env", default="package_env.json",
                      help="JSON file that defines package environment variables")
    (options, args) = parser.parse_args()
    return options, args


if __name__ == "__main__":
    (options, args) = parse_args()
    package(options)
