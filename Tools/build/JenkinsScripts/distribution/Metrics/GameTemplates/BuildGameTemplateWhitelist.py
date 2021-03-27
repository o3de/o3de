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

import BuildGameTemplateWhitelistArgs
import json
import os
import sys

# Generates a whitelist of project template names we can include in metrics.
# Input is a path to the project templates folder for a Lumberyard install.
# Output is a JSON formatted file in that directory, that will be included in the package.
# A template is defined by a templatedefinition.json file inside of a folder, the template name is the folder containing this file.
# Output should look like:
# {
#   "TemplateListDescription": "This is a list of Amazon provided templates. This is a white list of project template names that are included in usage event tracking. If a template name is not in this list, the name will not be included in the event.",
#   "TemplateNameWhitelist": [
#     "SimpleTemplate",
#     "EmptyTemplate"
#   ]
# }
def main():
    print "Generating Game Template Whitelist"
    args = BuildGameTemplateWhitelistArgs.createArgs()
    
    validTemplates = []
    # Search for project templates.
    for root, dirnames, filenames in os.walk(args.projectTemplatesFolder):
        templateRoot = os.path.basename(root)
        for file in filenames:
            if file == args.templateDefinitionFileName:
                fullPath = os.path.join(templateRoot, file)
                print "Found template definition: " + fullPath
                validTemplates.append(templateRoot)

    templateDescriptionKey = "TemplateListDescription"
    templatedescriptionValue = "This is a white list of project template names that are included in usage event tracking. "
    templatedescriptionValue  += "If a template name is not in this list, the name will not be included in the event."

    TemplateNameWhitelistKey = "TemplateNameWhitelist"
    jsonString = json.dumps({templateDescriptionKey: templatedescriptionValue, 
                             TemplateNameWhitelistKey: validTemplates},
                            sort_keys=True,
                            indent=4,
                            separators=(',', ': '))

                            
    templateWhitelistFilePath = os.path.join(args.projectTemplatesFolder, args.templateWhitelistFilename)
    try:
        whitelistFile = open(templateWhitelistFilePath, 'w')
        whitelistFile.write(jsonString)
        whitelistFile.close()
    except:
        print "Error writing template whitelist to file " + templateWhitelistFilePath
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main())
