"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from optparse import OptionParser
import os, json, re, sys

dirDescriptor = ".package.dir"
def main():
    parser = OptionParser()
    parser.add_option(  "-s", "--source",
                        dest="source",
                        help="Specify the boot-strap tool's metadata file to be parsed (i.e. C:/dev/SetupAssistantConfig.json)",
                        default="../../../../SetupAssistantConfig.json")
    parser.add_option(  "-o", "--outputfile",
                        dest="output",
                        help="Specify the output file of this tool.  If it exists, it will be over-written.",
                        default="./3rdparty_versions.txt")
    (options, args) = parser.parse_args()

    if not os.path.isfile(options.source):
        print 'invalid sourcefile "{}"'.format(options.source)
        return 2

    ant_property_list = ''

    with open(options.source, 'r') as source:
        source_json = json.load(source)

        sdks_list = source_json['SDKs']

        for sdk_object in sdks_list:
            identifier = sdk_object['identifier'].encode('ascii')

            if identifier:
                subdir = sdk_object.get('source')

                ant_property_list += '{}={}\n'.format(identifier + dirDescriptor, subdir)
                # Wwise LTX is distributed differently than other SDKs, and exists in two locations.
                if identifier == "wwiseLtx":
                    justVersionDir = re.sub("Wwise/", "", subdir)
                    ant_property_list += '{}={}\n'.format("wwiseLtx.tool" + dirDescriptor, justVersionDir)

    with open(options.output, 'w') as output:
        output.write(ant_property_list)

    print '\nANT Properties:'
    print ant_property_list.strip()

if __name__ == "__main__":
    main()
