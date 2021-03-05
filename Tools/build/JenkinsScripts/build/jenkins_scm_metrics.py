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
All this script is doing is writing grabbing a file that was written previously marking the start of when the Perforce would run
and then getting the current time to find out how long we spent in Perforce
'''

import time

from utils.util import *


def write_metrics():
    enable_build_metrics = os.environ.get('ENABLE_BUILD_METRICS')
    metrics_namespace = os.environ.get('METRICS_NAMESPACE')
    if enable_build_metrics == 'true':
        scm_end = int(time.time())
        workspace = os.environ.get('WORKSPACE')
        metrics_file_name = 'scm_start.txt'
        if workspace is None:
            safe_exit_with_error('{} must be run in Jenkins job.'.format(os.path.basename(__file__)))
        try:
            with open(os.path.join(workspace, metrics_file_name), 'r') as f:
                scm_start = int(f.readline())
        except:
            safe_exit_with_error('Failed to read from {}'.format(metrics_file_name))

        scm_total = scm_end - scm_start

        script_path = os.path.join(workspace, 'dev/Tools/build/waf-1.7.13/build_metrics/write_build_metric.py')

        build_tag = os.environ.get('BUILD_TAG')
        p4_changelist = os.environ.get('P4_CHANGELIST')

        if build_tag is not None and p4_changelist is not None:
            os.environ['BUILD_ID'] = '{0}.{1}'.format(build_tag, p4_changelist)

        cwd = os.getcwd()
        os.chdir(os.path.join(workspace, 'dev'))
        cmd = 'python {} SCMTime {} Seconds --enable-build-metrics {} --metrics-namespace {} --project-spec None'.format(script_path, scm_total, True, metrics_namespace)
        # metrics call shouldn't fail the job
        safe_execute_system_call(cmd, shell=True)
        os.chdir(cwd)


if __name__ == "__main__":
    write_metrics()
