"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import sys

def printError(message):
    print(message)
    sys.exit(1)


def reportIterationStatus(index, count, reportFrequency, message):
    # Reporting on the first, last, and at a frequency that is a good balance of not spamming the logs
    frequency = count / reportFrequency
    lastPercent = float(index-1) / float(count)
    percentComplete = float(index) / float(count)
    lastReportSlice = int(lastPercent * frequency)
    thisReportSlice = int(percentComplete * frequency)
    shouldPrint = lastReportSlice != thisReportSlice or index == 1 or index == count
    if shouldPrint:
        print "\t{0}% complete, {1}/{2} {3}".format(int(percentComplete*100), index, count, message)
