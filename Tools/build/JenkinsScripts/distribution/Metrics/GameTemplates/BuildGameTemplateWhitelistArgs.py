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

import argparse

def createArgs():
    parser = argparse.ArgumentParser(description='Builds a whitelist of Game Templates to include in reported metrics.')
    parser.add_argument('--projectTemplatesFolder', 
                        default=None, 
                        required=True, 
                        help='Path to the templates folder. Expected: dev\\ProjectTemplates\\')
    parser.add_argument('--templateDefinitionFileName', 
                        default='templatedefinition.json', 
                        help='Template definition file name (default templatedefinition.json)')
    parser.add_argument('--templateWhitelistFilename', 
                        default='TemplateListForMetrics.json', 
                        help='Template whitelist file name (default TemplateListForMetrics.json)')
    args = parser.parse_args()
    print "Building game template whitelist with the following arguments"
    print(args)
    return args

