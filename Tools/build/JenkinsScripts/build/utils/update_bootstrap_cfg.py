"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

'''
This script is to update the configuration in dev/bootstrap.cfg
Usage: python update_bootstrap_cfg.py --bootstrap_cfg file_path --replace key1=value1,key2=value2
'''

from optparse import OptionParser
import os
import stat


def update_bootstrap_cfg(file, replace_values):
    try:
        with open(file, 'r') as bootstrap_cfg:
            content = bootstrap_cfg.read()
    except:
        error('Cannot read file {}'.format(file))
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
        with open(file, 'w') as out:
            out.write('\n'.join(new_content))
    except:
        error('Cannot write to file {}'.format(file))
    print '{} updated with value {}'.format(file, replace_values)


def error(msg):
    print msg
    exit(1)


def parse_args():
    parser = OptionParser()
    parser.add_option("--bootstrap_cfg", dest="bootstrap_cfg", default=None, help="File path of bootstrap.cfg to be updated.")
    parser.add_option("--replace", dest="replace", default=None, help="Target platform to package")
    (options, args) = parser.parse_args()
    bootstrap_cfg = options.bootstrap_cfg
    replace = options.replace

    if not bootstrap_cfg:
        error('bootstrap.cfg is not specified.')
    if not os.path.isfile(bootstrap_cfg):
        error('File {} not found.'.format(bootstrap_cfg))
    replace_values = {}
    if replace:
        try:
            replace = replace.split(',')
            for r in replace:
                r = r.split('=')
                key = r[0].strip(' ')
                value = r[1].strip(' ')
                replace_values[key] = value
        except IndexError:
            error('Please check the format of argument --replace.')

    return bootstrap_cfg, replace_values


if __name__ == "__main__":
    (file, replace_values) = parse_args()
    update_bootstrap_cfg(file, replace_values)
