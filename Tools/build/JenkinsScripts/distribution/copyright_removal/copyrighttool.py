# This script updates all copyright messages in the code to the Amazon standard copyright messages
# cleans up Crytek original messages (if present) and preserves copyright messages from any other 3rd party.

# The tool must be run from the root of the workspace corresponding to the branch selected below.
# You can choose either to replace all copyright messages, or skip files that already have been stamped.
# If the official copyright message has changed, you should run the tool to apply to all files.
# The tool will strip old Amazon headers, and replace them with new ones.

# CONFIGURATION VARIABLES
# These variables change the behavior of the script
# The root of the perforce branch to operate in

perforceRoot="//lyengine/dev"

# The root of the directory to actually scan for copyright updates. This may be below the branch root.
# Normally you only pick something below the branch root in order to do a quick scan of the source
# while making changes to this script.
scanRoot="//lyengine/dev"

# Test paths used during development
#scanRoot="//lyengine/dev/Code/CryEngine/CryCommon"
#scanRoot="//lyengine/dev/Code/Sandbox/Editor"
#scanRoot="//lyengine/dev/Code/Sandbox/Plugins/EditorCommon/Serialization"

# Skip files that already have the Amazon standard notice. Useful for incremental runs of the tool.

skipFilesWithAmazonNotice=True
#skipFilesWithAmazonNotice=False

# Bare text of the current official notice. This gets wrapped in comment markers depending on the
# type of file it is applied to.

OfficialNotice = """
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""


# END OF CONFIGURATION VARIABLES


import os
import os.path
import re
import shutil
import subprocess

# We will only apply copyright notices to files in this list, unless Crytek asserts a copyright in the file
# This is a list of those suffixes
codeExtensions = [
    'cpp',    # C++
    'cc',    # Objective C
    'c',    # C
    'cs',    # C#
    'mm',    # C++ file for Objective C compiler
    'hpp',    # c++ include
    'hxx',    # c++ include
    'h',    # C or C++ include
    'inl',    # C++ inline include file
    #'rc',    # MFC resource file
    #'bat',    # Batch file
    #'sh',    # Shell script
    #'py',    # Python file
    #'lua',    # Lua file
    'ext',    # Material description file
    'cfi',    # Shader language include
    'cfx',    # Shader language
]

# This pattern matches those suffixes
codeFilePat = re.compile("^.*\.(" + "|".join(codeExtensions) + ")$",re.IGNORECASE) 

def partition_files(root):

    vendorFiles = {}

    # Stage 1: sometimes we copied files directly into the tree without branching, so we start out by getting
    # a dictionary of all file path suffixes that occur in vendor branches under /lyengine/vendor
    # Each of these branches is of the form //lyengine/vendor/<name>, which we strip off to generate the suffix
    # We do this with a single p4 command for speed.

    print "Fetching vendor files dictionary"

    p = subprocess.Popen("p4 files //lyengine/vendor/...",shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    pat = re.compile("^//lyengine/vendor/[^/]*/(.*)#.*")
    for line in p.stdout:
        line = line.strip()
        m = pat.match(line)
    if m:
        vendorFiles[m.group(1)] = 1

    # Stage 2, we look at all files under the provided root and check if the suffix minus the root is a file
    # that occurs in a crytek original branch.

    print "Identified {0} unique file paths in original CryEngine branches(s).".format(len(vendorFiles))

    print "Scanning files under: " + root

    # Now we scan the non-deleted files under our root (note the "p4 files -e" flag to skip deleted files!)
    # For each of these files, we check if it was originally in a vendor drop or not
    # partitioning into two lists, amazonFiles and crytekFiles.
    # This is the first stage of determining which files get which copyright message.
    # Because we may also have open source files, we must do further investigations to see
    # which case we are in with each file.

    p = subprocess.Popen("p4 files -e " + root + "/...",shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    pat = re.compile("^" + perforceRoot + "/(.*)#.*")
    epat = re.compile("^.*/(SDKs|sdk|waf-.*)/")

    for line in p.stdout:
        line = line.strip()

    # Skip files that are in external SDK directories, in the waf source or are not known code file types
    if epat.match(line):
        continue

        m = pat.match(line)
    if m:
        suffix = m.group(1)
        # Skip files that are not code files unless they have a crytek copyright
        if not codeFilePat.match(suffix) and not hasCrytekCopyrights(suffix):
        continue

        if suffix in vendorFiles:
            crytekFiles[suffix] = "Pure"
        else:
            amazonFiles[suffix] = "Pure"

    for file in amazonFiles.keys():

    if skipFilesWithAmazonNotice and hasAmazonNotice(file):
        amazonFiles[file] = "Skip"
    elif has3rdPartyCopyrights(file):
        amazonFiles[file] = "3rdParty"

    for file in crytekFiles.keys():
    if skipFilesWithAmazonNotice and hasAmazonNotice(file):
        crytekFiles[file] = "Skip"
    elif has3rdPartyCopyrights(file):
        crytekFiles[file] = "3rdParty"

    return

# This function scans a file to see if there are copyright notices or license notices from Crytek
def hasCrytekCopyrights(file):
    try:
        count = 0
        f=open(file)
        p = re.compile(r".*\bcopyright\b|(c) ?[12][0-9][0-9][0-9]",re.IGNORECASE)
        pCrytek = re.compile(r".*\bcrytek\b",re.IGNORECASE)
        for line in f:
            if p.match(line) and pCrytek.match(line):
            return True
        count += 1
        # Optimization. Crytek copyrights occur early in the file. Don't look too far into binary files.
        if count > 100:
        return False
        return False
    except:
        return False

# This function scans a file to see if there are official copyright headers from Amazon
def hasAmazonNotice(file):
    try:
        count = 0
        f=open(file)

    # Break up the notice lines to check in order
    lines = OfficialNotice.strip('\n').splitlines()
    # Index of the currently matched line for the whole notice
    matchIndex = 0

        for line in f:
        line = line.strip('\n')
            if line.endswith(lines[matchIndex]):
            matchIndex += 1
        if matchIndex == len(lines):
                return True

        count += 1
        # Optimization. Amazon copyrights occur early in the file. Don't look too far into binary files.
        if count > 100:
        return False
        return False
    except:
        return False

# This function scans a file to see if there are copyright notices or license notices
# from some party other than Amazon or Crytek
def has3rdPartyCopyrights(file):
    try:
        f=open(file)
        p = re.compile(r".*\bcopyright\b|(c) ?[12][0-9][0-9][0-9]",re.IGNORECASE)
        pAmazon = re.compile(r".*\bamazon\b",re.IGNORECASE)
        pCrytek = re.compile(r".*\bcrytek\b",re.IGNORECASE)
        pWAITING = re.compile(r".*COPYRIGHT_NOTICE_TBD",re.IGNORECASE)
        for line in f:
            if p.match(line) and not pAmazon.match(line) and not pCrytek.match(line) and not pWAITING.match(line):
            return True
        return False
    except:
        return False

# We define some patterns to find and remove Amazon copyright header blocks here

# This pattern must occur in every commment block we want to erase

removeBlock=re.compile(r".*COPYRIGHT_NOTICE_TBD|.*Copyright.*Amazon.com",re.IGNORECASE)

# Matches any # or // style comment
CStartBlock = re.compile(r"[ \t]*/\*")
CStopBlock = re.compile(r".*\*/[ \t]*\n")
XMLStartBlock = re.compile(r"[ \t]*<!--")
XMLStopBlock = re.compile(r".*-->[ \t]*\n")
CPPComment = re.compile(r"[ \t]*//")
LuaComment = re.compile(r"[ \t]*//")
BatchComment = re.compile(r"[ \t]*#(?!(def|if|endif|undef|pragma|include))")

CrytekCopyright = re.compile(r"^.*(crytek.*copyright|crytek.*\(c\)|copyright.*crytek|\(c\).*crytek)",re.IGNORECASE)

EngineIDLine = re.compile(r"^(.*)\b(Crytek Engine|Crytek CryEngine|(?<!Crytek )CryEngine)\b(.*)",re.IGNORECASE)


# This function applies a notice at the start of a copyright file.
# It also removes any Amazon block copyright notices, so that they can be replaced
# with the refreshed block

def startOrContinueBlock(line,state):
    if state=="code":
        if CStartBlock.match(line):             state = "cblock"
        elif XMLStartBlock.match(line):            state = "xmlblock"
    elif BatchComment.match(line):             state = "batch"
    elif CPPComment.match(line):             state = "cpp"
    elif LuaComment.match(line):             state = "lau"

    elif state=="cblock" and CStopBlock.match(line):    state = "finish"

    elif state=="xmlblock" and XMLStopBlock.match(line): state = "finish"

    elif state=="batch":
        if BatchComment.match(line):            state = "batch"
    else:                        state = "code"

    elif state=="cpp":
        if CPPComment.match(line):            state = "cpp"
    else:                        state = "code"

    elif state=="Lua":
        if LUAComment.match(line):            state = "lua"
    else:                        state = "code"
        
    return state

def createNotice(bareNotice,commentStart,commentEnd,commentContinue):
    result = commentStart + commentStart[-1:]*100 + "\n"
    lines = bareNotice.strip('\n').splitlines()
        result += commentContinue + "\n"
    for line in lines:
         result += commentContinue + line + "\n"
        result += commentContinue + "\n"
    result += commentEnd[0:1]*100 + commentEnd + "\n"
    return result

COfficialNotice = createNotice(OfficialNotice,"/**","**/","* ")
CPPOfficialNotice = createNotice(OfficialNotice,"///","///","// ")
XMLOfficialNotice = createNotice(OfficialNotice,"<!-- -","- -->","-- ")
LuaOfficialNotice = createNotice(OfficialNotice,"---","---","-- ")
BatchOfficialNotice = createNotice(OfficialNotice,"###","###","# ")

def cleanLineofCrytek(line):
    # Clean away original Crytek copyright notices
    if CrytekCopyright.match(line):
    line = re.sub(r"^([ \t]*)(//|#|)([ \t]*).*$", r"\1\2\3",line)
    # Clean up references to CryEngine or Crytek Engine -> Lumberyard in block headers
    if EngineIDLine.match(line):
    line = EngineIDLine.sub(r"\1Lumberyard\3",line)
    return line


def applyCopyrightNotice(file,extraNotice):

    # Check out for edit
    p = subprocess.Popen("p4 edit \"" + file + "\"",shell=True)
    p.wait()

    f=open(file)
    fo=open("tmpfile","w")

    content = ""
    count = 0

    # Grab the first 50 lines to work with
    # The rest we copy over without looking at them

    block=""
    state="code"
    remove=False
    placeBlock=0

    for line in f:

    # Try to put the comment underneath any include guards and pragma once statement
    if placeBlock <=3 and re.match("^[ \t]*\n",line): placeBlock = placeBlock
    elif placeBlock == 0 and re.match("^#ifndef ",line): placeBlock = 1 
    elif placeBlock == 1 and re.match("^#define ",line): placeBlock = 2
    elif placeBlock == 2 and re.match("^#pragma once",line): placeBlock = 3
    elif placeBlock <= 4: placeBlock = 4

    if placeBlock == 4:
        placeBlock = 5
        # Place the official notice and the extra notice in place depending on file extension
        extension = os.path.splitext(file)[1].lower()
        if extension in ['.c','.h']:
        fo.write(COfficialNotice)
        if extraNotice:
            fo.write("\n/* {0} */\n".format(extraNotice))
        elif extension in ['.cpp','.cc','.cs','.mm','.hpp','.hxx','.inl','.rc','.ext','.cfx','.cfi']:
        fo.write(CPPOfficialNotice)
        if extraNotice:
            fo.write("\n// {0}\n".format(extraNotice))
        elif extension == ".lua":
        fo.write(LuaOfficialNotice)
        if extraNotice:
            fo.write("\n-- {0}\n".format(extraNotice))
        elif extension == ".targets": # XML style comments
        fo.write(XMLOfficialNotice)
        if extraNotice:
            fo.write("\n<!-- {0} -->\n".format(extraNotice))
        else:
        fo.write(BatchOfficialNotice)
        if extraNotice:
            fo.write("\n# {0}\n".format(extraNotice))

    # After placig the main header
    # Eat blank lines
    if placeBlock == 5:
         if line == "\n": continue
         else: placeBlock = 6

    count += 1
    state = startOrContinueBlock(line,state)

    if state != "code" and state != "finish":
        line = cleanLineofCrytek(line)
        if line != "\n":
            block+=line
        if removeBlock.match(line): remove=True
    else:
        if state == "finish":
        line = cleanLineofCrytek(line)
            if line != "\n":
                block+=line
        if block != "":
        if not remove: fo.write(block)
        remove=False
        block=""
        if state != "finish":
            fo.write(line)
        state = "code"

    if count > 50 and state == "code":
        break

    # If we get out of the loop and we are in a block, the file ended
    if state != "code" and block != "":
    if not remove: fo.write(block)
    remove=False
    block=""

    # and now copy the rest of the file as is
    for line in f:
        fo.write(line)

    f.close()
    fo.close()

    # Make a backup
    #shutil.copyfile(file,file+".bak")
    # Make the change
    shutil.copyfile("tmpfile",file)


# Set up two lists to contain files originating with either Crytek or Amazon
# Each of these dictionaries will either have "Pure" or "3rdParty" for each file.
# Pure denotes files that have no 3rd party copyright notices associated with them.
# 3rdParty indicates a copyright message for some other party is present.

amazonFiles = {}
crytekFiles = {}


# Partition the files by copyright notice conditions
partition_files(scanRoot)

print "Done partition: classified {0} files.".format(len(amazonFiles)+len(crytekFiles))


# Apply the appropriate copyright notices to each file

changeCount = 0
skipCount = 0

# log the files that we modify
amzn_pure = open('copyright_removal_amzn_pure.log', 'w')
amzn_3rdparty = open('copright_removal_amzn_3rdparty.log', 'w')
amzn_skip = open('copyright_removal_amzn_skip.log', 'w')
crytek_pure = open('copyright_removal_crytek_pure.log', 'w')
crytek_copyright_found = open('copyright_removal_crytek_copyright_found.log', 'w')
crytek_3rdparty = open('copyright_removal_crytek_3rdparty.log', 'w')
crytek_skip = open('copyright_removal_crytek_skip.log', 'w')


for file in amazonFiles.keys():
    if amazonFiles[file] == "Pure":
        print "Apply Amazon Copyright: " + file
        applyCopyrightNotice(file,"")
        amzn_pure.write(file + '\n')
        changeCount += 1
    elif amazonFiles[file] != "Skip":
        print "Apply Amazon 3rd party notice: " + file
        applyCopyrightNotice(file,"Modifications copyright Amazon.com, Inc. or its affiliates.")
        amzn_3rdparty.write(file + '\n')
        changeCount += 1
    else:
        amzn_skip.write(file + '\n')
         skipCount += 1

for file in crytekFiles.keys():
    if crytekFiles[file] == "Pure":
        print "Apply Amazon sublicense of Crytek Notice: " + file
        # In this case, even if Crytek did not assert copyright, we will do it for them?
        applyCopyrightNotice(file,"Original file Copyright Crytek GMBH or its affiliates, used under license.")
        crytek_pure.write(file + '\n')
        changeCount += 1
    elif crytekFiles[file] != "Skip":
        # in this case, double check if there is a crytek notice in the file
        # and if the copyright notice that is there is near the top
        if hasCrytekCopyrights(file):
            print "Apply Amazon 3rd party notice and Crytek notice: " + file
            applyCopyrightNotice(file,"Original file Copyright Crytek GMBH or its affiliates, used under license.")
            crytek_copyright_found.write(file + '\n')
            changeCount += 1
        else:
            # This is a weird case. Crytek may or may not have changed the file, and have not asserted copyright.
            # We assume that they did not, because it is all from some other party.
            # These cases are few and probably bear investigation
            print "Apply Amazon 3rd party notice and Crytek notice of origin?: " + file
            applyCopyrightNotice(file,"Modifications copyright Amazon.com, Inc. or its affiliates.")
            crytek_3rdparty.write(file + '\n')
            changeCount += 1
    else:
        crytek_skip.write(file + '\n')
        skipCount += 1

amzn_pure.close()
amzn_3rdparty.close()
amzn_skip.close()
crytek_pure.close()
crytek_copyright_found.close()
crytek_3rdparty.close()
crytek_skip.close()



print "Done editing. {0} files checked out and changed.".format(changeCount)
if skipCount > 0:
    print "Skipped {0} files with existing Amazon notices.".format(skipCount)
