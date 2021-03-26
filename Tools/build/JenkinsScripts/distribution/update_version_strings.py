"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import argparse
import datetime
import os
import re
import shutil
import stat
import sys

# Files requiring embedded version string
waf_default_settings = "./dev/_WAF_/default_settings.json"

def update_version_strings(args):
        current_version = fetch_current_version(args)
        print 'Storing version: ' + current_version +" to " + waf_default_settings

        # preserve the developer's work and make it re-runnable
        suffix = datetime.datetime.now().strftime("%y%m%d_%H%M%S")
        tempfn = waf_default_settings + '-' + suffix

        shutil.move(waf_default_settings,tempfn) #preserve file attribs with move not copy
        shutil.copyfile( tempfn, waf_default_settings) # make a writable copy for ourselves
        print 'Original json settings saved to ' + tempfn

        with open( waf_default_settings, "r+" ) as file:
            fc = file.read() # currently default_settings.json is small enough to slurp
            fc = re.sub( r'(\"Build\s+Options"\s*:\s*\[[^\]]+\"default_value.+\")((\d+\.){2,4}\d+)\"', r'\g<1>' + current_version + '"', fc, flags=re.IGNORECASE|re.MULTILINE )
            file.seek( 0 ) # empty the file at byte 0
            file.truncate()
            file.write( fc )
        return

def fetch_current_version(args):
    changelist_number = int(args.changelist_number)
    # Below is how Lumberyard fits a changelist > 64K into a windows compatible version string that maxes out at 64K
    upper_word = (changelist_number >> 16) & 0xFFFF
    lower_word = changelist_number & 0xFFFF
    return (args.major + '.' + args.minor + '.' + str(upper_word) + '.' + str(lower_word))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('changelist_number', help='changelist number to embed into dlls')
    parser.add_argument('major', help='major version number')
    parser.add_argument('minor', help='minor version number')
    args = parser.parse_args()
    # Inability to set the version string is a warning not an error. Do not stop the build.
    try:
        update_version_strings(args)
    except:
        print "FATAL ERROR: Unable to set version string in " + waf_user_settings + ": check for file not found, not writable, or version= option not found"
        sys.exit(1)

if __name__ == '__main__':
    main()