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
#!/c/python27/python
import sys
import re

# Utility function to copy file contents to stdout
def copyFileToStdout(file):
    with open(file) as f:
    for x in f:
        sys.stdout.write(x)

# Search the filegroups.,txt file to find what doxygen group the content of the file should belong to
def findGroup(filename):
    lookupFile = sys.argv[1]
    lookupFile = re.sub("^.*/Code/","",lookupFile)
    lookupFile = re.sub("\.\./","",lookupFile)

    #sys.stderr.write("Lookup {0}\n".format(lookupFile))
    group = "Default"
    with open('filegroups.txt','r') as f:
    for x in f:
        x = x.rstrip()
        p = str.split(x,':')
        if p[0] in lookupFile:
        group = p[1]
        break
    if group == "Default":
    # If there is no group specified for this file in our map, then
    # use the file structure to place it under the Default grouping for now
        group = "_" + re.sub("[\./]","_",lookupFile) + "_"
    sys.stderr.write("\nCreating new file group: {0}\n".format(group))
    return group

def main():

    if "/Doxygen/" in sys.argv[1]:
    # We do nothing to files in the Doxygen source input directories
    # Just copy back to stdout in this case
    copyFileToStdout(sys.argv[1])
    else:
        processCommentBlock.counter = 0

        # Determine what group the file is
        group = findGroup(sys.argv[1])

    # And process the file to add the group markup
    # as well as update from DocOMatric syntax comments to doxygen
    #sys.stderr.write ("Updating file {0} to group {1}\n".format(sys.argv[1],group));
    sys.stdout.write ("/*! @addtogroup {0}\n * @{{ */\n".format(group))
    transformToDoxygen(sys.argv[1])
    sys.stdout.write("\n/*! @} */\n");

def transformToDoxygen(file):
    # Put together a matcher that will help us find comment blocks
    commentBlockPattern = re.compile('^[ \t]*//')
    commentTrailerPattern = re.compile('[;,][ \t]*//')

    block=""
    with open(file,'r') as f:
    for line in f:
        # Accumulate a comment block
        if commentBlockPattern.search(line):
        block += line
        else:
            processCommentBlock(block)
        block = ""
            if commentTrailerPattern.search(line):
            line = re.sub("//","///",line,1)
        sys.stdout.write(line)

    # output any final comment block
        processCommentBlock(block)


def processCommentBlock(block):
    processCommentBlock.counter += 1
    if block != "":
    # Avoid changing the file header block into documentation
    if re.search("//[ /t]*([sS]ummary|[Dd]escription|DOC-IGNORE)",block) and processCommentBlock > 1:
        # Start out with the simple global changes
        block = re.sub("//","///",block)

        # Get rid of some comment styles that are inserting lines into the comments
        # # e.g //-------------
        # //===========
        # //****************************
        # and so forth
        block = re.sub("///=+[ \t]*\n","///\n",block)
        block = re.sub("///-+[ \t]*\n","///\n",block)
        block = re.sub("///_+[ \t]*\n","///\n",block)
        block = re.sub("///\*+[ \t]*\n","///\n",block)
        block = re.sub("////+[ \t]*\n","///\n",block)

        # Transform some Docomatic text values into doxygen style comments instead
        block = re.sub("///[ \t]*[Dd]escription[ \t]*:?","/// @details",block)
        block = re.sub("///[ \t]*[Ss]ummary[\t ]*:?","/// @brief ",block)
        block = re.sub("///[ \t]*[Nn]otes[\t ]*:?","/// @note ",block)
        block = re.sub("///[ \t]*[Rr]emarks[\t ]*:?","/// @remark ",block)
        block = re.sub("///[ \t]*[Ss]ee [Aa]lso[\t ]*:?","/// @sa ",block)
        block = re.sub("///[ \t]*[Rr]eturn [Vv]alue[\t ]*:?","/// @returns ",block)
        block = re.sub("///[ \t]*[Rr]eturns[\t ]*:?","/// @returns ",block)
        block = re.sub("///[ \t]*[Oo]utputs[\t ]*:?","/// @returns ",block)
        block = re.sub("///[ \t]*DOC-IGNORE-BEGIN","/// @cond IGNORE ",block);
        block = re.sub("///[ \t]*DOC-IGNORE-END","/// @endcond ",block);

        # Now iterate and deal with input parameters. A bit trickier.
        # We want to convert things that look like parameters after one of
        # the keywords Arugments, Inputs or Parameters, leading up to the next command.
        block = block.rstrip();
        list = str.split(block,'\n')
        # Zero out block, we will rebuild it as we iterate over list
        block = ""
        inparams = 0
        for line in list:
        if re.search("// @",line):
            inparams = 0
        elif re.search("///[ \t]*([Aa]rguments|[Ii]nputs|[Pp]arameters)",line):
            line = re.sub("///.*","///",line)
            inparams = 1
        elif inparams == 1 and re.search("///[ \t]*[A-Za-z0-9_]+[ \t]*[:-]",line):
            line = re.sub("///[ \t]*","/// @param ",line)
            line = re.sub("@param[ \t]*([A-Za-z0-9_]+)[ \t]*[-:]","@param \\1 ",line)
            block += line + "\n"
    sys.stdout.write(block)

if __name__ == "__main__": 
    main()

