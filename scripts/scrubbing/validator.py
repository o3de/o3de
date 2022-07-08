#!/usr/bin/env python
"""Python script to validate that code does not contain an console platform specific
references or code that should not be published. Can be used to scan any directory."""

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# pylint: disable-msg=C0301

from __future__ import absolute_import
from __future__ import print_function
import six
import logging
import os
import re
import sys
import traceback
import json
from pathlib import Path
from optparse import OptionParser
if six.PY2:
    from cStringIO import StringIO
else:
    from io import StringIO

import validator_data_LEGAL_REVIEW_REQUIRED # pull in the data we need to configure this tool

class Validator(object):
    """Class to contain the validator program"""
    # Set of all acceptable_use patterns actually used during the run
    pattern_used = set([])

    def validate_line(self, line, filepath, fileline, failed, errors, info):
        """Check that a line of text does not have any pattern that leaks IP."""
        # The acceptable_use_patterns must be kept up to date as the code base evolves, and it is desirable
        # that this list be short. The -u command line option will run the validator in a mode
        # where it will report acceptable use patterns that did not get used in the check.
        # Those patterns are candidates for elimination. Ultimately we want the acceptable use
        # pattern set to be empty, or as close to empty as possible.
        #
        # This is a 3 stage check. First we do a rapid check against fixed text
        # strings (self.prefilter). These strings must capture a superset of potential
        # problematic texts. The test is run by lowercasing all characters, so that
        # case specific strings are not required. For example "abc" tests for the
        # presence of any combination of the letters in any case.
        # This first stage is a speed optimization, because regular expression matching
        # in python is really horribly slow (orders of magnitude slower than unix
        # grep in many cases!).
        lower = line.lower()
        if any(ext in lower for ext in self.prefilter):
            line = line.rstrip(os.linesep)

            # Before we engage the accepted use machinery, make sure that the line contains a bad pattern that would
            # otherwise fail validation
            if self.compiled_bad_pattern.search(line) != None:

                # The second stage checks against a more specific regular expression. (self.compiled_acceptable_pattern)
                # if the regular expression is matched, then the line contains information
                # that may be acceptable use or a false positive. We check if the line is covered by an established
                # exception by the presence of matching pattern in the self.acceptable_use_patterns regular expression.
                # Make a copy of the line and replace any matching accepted uses with EXCEPTION. If something
                # changes, then we have a possible accepted use which we'll have to vet.
                origline = line
                line = self.compiled_acceptable_pattern.sub('EXCEPTION', line)
                accepting_patterns = []
                if line != origline:

                    # Convert the path to forward slashes, then loop through all instances of an accepted use match
                    # on this line
                    line = origline
                    canonical_filepath = os.path.abspath(filepath).replace('\\', '/')
                    while True:
                        m = self.compiled_acceptable_pattern.search(line)
                        if not m:
                            break

                        # Find the specific matching pattern
                        match = line[m.start():m.end()]
                        found = False
                        for pattern,compiledp,fileset in self.acceptable_use_patterns:
                            if compiledp.search(match) != None:

                                # If file testing isn't enabled, assume the match is good. Otherwise, search the file
                                # patterns to see if this file is allowed
                                if self.options.ignore_file_paths:
                                    found = True
                                else:
                                    for fp in fileset:
                                        if fp.search(canonical_filepath) != None:
                                            found = True
                                            break
                                    if not found:
                                        errors.append("File rejected by pattern '{}': {}: line {}: {}".format(
                                            pattern, filepath, fileline, origline))

                                # First pattern match is good enough
                                if found:
                                    if self.options.check_unused_patterns:
                                        self.pattern_used.add(pattern)
                                    accepting_patterns.append(pattern)
                                break

                        # If no match was vetted by the filename, stop processing the line.
                        if not found:
                            break

                        # We remove the accepted use from the line and replace it with "EXCEPTION" and retest the line.
                        line = line[:m.start()] + 'EXCEPTION' + line[m.end():]

                # Once any possible accepted uses have been replaced, we check the resultant line to see if any bad
                # patterns remain in the line. If so, then we fail the line.
                if self.compiled_bad_pattern.search(line) != None:
                    if self.options.list:
                        self.output_unique_filepath(filepath)
                    else:
                        errors.append('validation failure in {}: line {}:  {}'.format(filepath, fileline, origline))
                    failed = 1

                # Otherwise, spit out the details of each match
                else:
                    for a in accepting_patterns:
                        info.append("Allowed by '{}' pattern: {}: line {}: {}".format(a, filepath, fileline, origline))
                        if self.options.exception_file:
                            self.exceptions_output.write("Allowed by '{}' pattern: {}: line {}: {}\n".format(a, filepath, fileline, origline))
        return failed

    def output_unique_filepath(self, filepath):
        """Output the name of a file exactly once in a run."""
        # Dictionary to use to ensure that we know which files we have already talked about
        global printed_filepath
        try:
            if not filepath in printed_filepath:
                print(filepath)
                printed_filepath[filepath] = 1
        except:
            # Global dict does not exist yet. Make it.
            printed_filepath = {}
            printed_filepath[filepath] = 1
            # Just print the filepath if we have not already printed it
            print(filepath)

    def validate_file(self, filepath):
        """Validate the content of a file 'filepath'.
        Return 0 if no issues are found, and 1 if an issue was noted."""
        failed = 0

        # Otherwise read the file off disk.  Check the filename itself to make sure no naughty
        # bits are there.
        errors = []
        info = []
        failed = self.validate_line(filepath, 'filename', 0, failed, errors, info)
        for e in errors:
            logging.error(e)
        for i in info:
            logging.info(i)

        # Check if this file is a binary file, or an extension we always skip
        # These extensions are here because they sometimes look like text files,
        # but are not really text files in practice.
        if validator_data_LEGAL_REVIEW_REQUIRED.skip_file(filepath):
            logging.debug('Skipping %s', filepath)
            return

        # Python3 requires specific encoding but the repo is a mix of UTF-8, UTF-16, and latin-1
        # Just try except the possibilities until it works
        encodings = ["utf8", "utf-16-le", "utf-16-be", "latin-1"]
        for encoding_format in encodings:
            try:
                with open(filepath, encoding=encoding_format) as f:
                    logging.debug('Validating %s', filepath)

                    # Take care to deal with files that have unreasonably large lines.
                    # if we don't do this, then the validator can segfault as it tries
                    # to read a line that is insanely large. A try/except will not prevent this.
                    # This can happen, for example, in XML file or obj files (the 3d format, not
                    # the compiler output kind).
                    # Quesion: if we encounter such a file, should we call it "binary" and quit?
                    # TODO: There is a small "leak" here, in that we don't deal with crossing
                    # a non line boundary clealy. In particular, a validation pattern
                    # that occurs in the boundary crossing will not be properly searched.
                    # The easiest thing to do is probably to retain the prior 128 bytes or
                    # so from the prior portion of the line as a prefix to the tail of the rest of the line.
                    # This will make the logic below much more complex.
                    fileline = 0
                    line = f.readline(10000)
                    while line != '':
                        fileline += 1
                        errors = []
                        info = []
                        failed = self.validate_line(line, filepath, fileline, failed, errors, info)
                        for e in errors:
                            logging.error(e)
                        for i in info:
                            logging.info(i)
                        line = f.readline(10000)
                return failed
            except UnicodeDecodeError:
                continue

        raise UnicodeError("Could not decode {0} due to an unexpected file encoding".format(filepath))

    # Walk directory tree and find all file paths, and run the search for bad code on each file.
    # We explicitly skip "SDKs" directories, "BinTemp" and "Python" directories and various others.
    # The first two are never part of the package, and Python is a special case SDK that we ship as is.\
    # TODO: Perhaps the directories to skip should become a parameter so we can use the validator
    # on non-Lumberyard trees.
    def validate_directory_tree(self, root, platform):
        """Walk from root to find all files to validate and call the validator on each file.
        Return 0 if no problems where found, and 1 if any validation failures occured."""
        counter = 0
        platform_failed = 0
        scanned = 0
        validations = 0
        bypassed_directories = validator_data_LEGAL_REVIEW_REQUIRED.get_bypassed_directories(self.options.all)

        for dirname, dirnames, filenames in os.walk(os.path.normpath(root)):
            # First deal with the files in the current directory
            for filename in filenames:
                filepath = os.path.join(dirname, filename)
                scanned += 1
                file_failed = self.validate_file(os.path.normpath(filepath))
                if file_failed:
                    platform_failed = file_failed
                else:
                    validations += 1

            # Trim out allowlisted subdirectories in the current directory if allowed
            for name in bypassed_directories:
                if name in dirnames:
                    dirnames.remove(name)
        if scanned == 0:
            logging.error('No files scanned at target search directory: %s', root)
            platform_failed = 1
        else:
            print('validated {} of {} files'.format(validations, scanned))
        return platform_failed


    def compile_filter_patterns(self, platform):
        """Join together patterns listed in data file into single patterns and compile for speed."""
        if not platform in validator_data_LEGAL_REVIEW_REQUIRED.restricted_platforms:
            logging.error('platform data for platform %s not provided in validator_data_LEGAL_REVIEW_REQUIRED.py.', platform)
            sys.exit(1)
        restricted_platform = validator_data_LEGAL_REVIEW_REQUIRED.restricted_platforms[platform]
        if not 'prefilter' in restricted_platform:
            logging.error('prefilter list not found for platform %s in validator_data_LEGAL_REVIEW_REQUIRED.py', platform)
            sys.exit(1)
        self.prefilter = restricted_platform['prefilter']
        if not 'patterns' in restricted_platform:
            logging.error('patterns list not found for platform %s in validator_data_LEGAL_REVIEW_REQUIRED.py', platform)
            sys.exit(1)
        self.bad_patterns = restricted_platform['patterns']
        if not 'acceptable_use' in restricted_platform:
            logging.error('acceptable_use list not found for platform %s in validator_data_LEGAL_REVIEW_REQUIRED.py', platform)
            sys.exit(1)
        self.acceptable_use_patterns = []
        for p,fileset in restricted_platform['acceptable_use']:
            try:
                self.acceptable_use_patterns.append((p, re.compile(p), [re.compile(f) for f in fileset]))
            except:
                logging.error("Couldn't compile pattern {}...".format(p))
                traceback.print_exc()
                sys.exit(1)

        try:
            # Compile the search patterns for speed
            bad_pattern = '|'.join(self.bad_patterns)
            acceptable_pattern = '|'.join([p for p,compiledp,fileset in self.acceptable_use_patterns])
            self.compiled_bad_pattern = re.compile(bad_pattern)
            self.compiled_acceptable_pattern = re.compile(acceptable_pattern)
        except:
            logging.error('Could not compile patterns for validation. Check patterns in validator_data_LEGAL_REVIEW_REQUIRED.py for correctness.')
            traceback.print_exc()
            sys.exit(1)

    def test_prefilter_covers_bad_patterns(self):
        double_pattern = re.compile(r'\[(.)\1\]')
        for bad in self.bad_patterns:
            reduced = re.sub(double_pattern, r'\1', bad.lower())
            found = False
            for p in self.prefilter:
                if p in reduced:
                    found = True
                    break
            if not found:
                logging.error('Could not find a prefilter for {}.'.format(bad))
                return False
        return True

    def test_all_bad_patterns_active(self, platform):

        # Open the canary file relative to the validator script, that way we don't have to worry about temporary files and whatnot
        # Once we split the repos we will have to worry about multiple root points etc. but that is a problem for future us.
        # All the packaging safelist stuff goes away once repo is split for platforms
        this_path = Path(__file__).resolve()
        root_folder = this_path.parents[2]
        relative_folder = os.path.relpath(this_path.parent, root_folder)
        canary_file = os.path.join(root_folder, 'restricted', platform, relative_folder, platform.lower() + '_canary.txt')
        try:
            with open(canary_file) as canary:
                bad_patterns = self.bad_patterns
                canary.seek(0, 0)
                fileline = 0
                for line in canary:
                    fileline += 1
                    errors = []
                    info = []

                    # Each validation failure needs to be tracked back to the patterns that detect it. Once we find one, eliminate it
                    # from the search list since we've found an instance where it would trigger
                    if self.validate_line(line, canary_file, fileline, 0, errors, info) == 1:
                        found = []
                        for bad in bad_patterns:
                            if re.search(bad, line):
                                found.append(bad)
                        for bad in found:
                            bad_patterns.remove(bad)

                # If the search list isn't empty, then whatever remains needs a canary
                if len(bad_patterns) > 0:
                    for bad in bad_patterns:
                        logging.error("Could not find a canary for '{}'.".format(bad))
                    return False
        except:
            logging.error('Could not open canary file {}.'.format(canary_file))
            return False
        return True

    def test(self, platform):
        if not self.test_prefilter_covers_bad_patterns():
            return False
        if not self.test_all_bad_patterns_active(platform):
            return False
        return True

    def validate(self, platform):
        self.compile_filter_patterns(platform)
        if not self.test(platform):
            logging.error('Validation could not pass {} self tests! Results cannot be trusted!'.format(platform))
            sys.exit(1)
        else:
            print('{} self tests SUCCEEDED'.format(platform))

        # Add the source code / SDK paths to check
        platform_failed = self.validate_directory_tree(os.path.abspath(self.args[0]), platform)

        # If the user asked, output any acceptable_use patterns that didn't get used
        if self.options.check_unused_patterns:
            for pattern,compiledp,fileset in self.acceptable_use_patterns:
                if not pattern in self.pattern_used:
                    print("UNUSED ACCEPTABLE_USE PATTERN: '{}'".format(pattern))

        return platform_failed

    def __init__(self, options, args):
        self.options = options
        self.args = args
        self.prefilter = None
        self.compiled_bad_pattern = None
        self.compiled_acceptable_pattern = None
        self.bad_patterns = None
        self.acceptable_use_patterns = None

def parse_options():
    """Set up the options parser, and parse the options the user gave to validator."""
    usage = 'usage: %prog [options] scandir'
    parser = OptionParser(usage)
    platform_choices = list(validator_data_LEGAL_REVIEW_REQUIRED.restricted_platforms_for_package.keys())
    platform_choices.sort()
    parser.add_option('--package_platform', action='store', type='choice',
                      choices=platform_choices,
                      dest='package_platform',
                      help='Package platform to validate. Must be one of {}.'.format(platform_choices))
    parser.add_option('-s', '--store-exceptions', action='store', type='string', default='',
                      dest='exception_file',
                      help='Store list of lines that the validator gave exceptions to by matching accepted use patterns. These can be diffed with prior runs to see what is changing.')
    parser.add_option('-v', '--verbose', action='store', type='choice', choices=['0', '1', '2'], default='0',
                      dest='verbose',
                      help='Verbose output. Level 0 = only output lines that fail validation. '
                           'Level 1 = output of lines that would have failed without an accepted usage exception. '
                           'Level 2 also includes output of each filename being handled.')
    parser.add_option('-u', '--check-unused-patterns', action='store_true',
                      dest='check_unused_patterns',
                      help='Report on any acceptable_use patterns that are not matched.')
    parser.add_option('-l', '--list', action='store_true',
                      dest='list',
                      help='Only list filenames with validation errors. Useful as input to a set of files to edit or otherwise process.')
    parser.add_option('-a', '--all', action='store_true',
                      dest='all',
                      help='Do not skip any files or subdirectories when processing. Should be used on final clean code only. If you use this on your build tree in place lots of temp files will match.')
    parser.add_option('-i', '--ignore-file-paths', action='store_true',
                      dest='ignore_file_paths',
                      help='disable the filepath check for accepted_use patterns. Should only be when targeting a directory other than /dev/.')

    (options, args) = parser.parse_args()

    if options.verbose == '1':
        logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.INFO)
    elif options.verbose == '2':
        logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.DEBUG)
    else:
        logging.basicConfig(format='%(levelname)s: %(message)s')

    if len(args) != 1:
        parser.error('no directory to scan specified and/or incorrect number of directories specified.')
    return options, args

def main():
    """Main function for validator script"""
    options, args = parse_options()
    validator = Validator(options, args)

    package_failed = 0
    package_platform = validator.options.package_platform
    prohibited_platforms = validator_data_LEGAL_REVIEW_REQUIRED.get_prohibited_platforms_for_package(package_platform)

    if validator.options.exception_file != '':
        try:
            validator.exceptions_output = open(validator.options.exception_file, 'w', errors='ignore')
        except:
            logging.error("Cannot open exceptions output file '%s'", validator.options.exception_file)
            sys.exit(1)

    for platform in prohibited_platforms:
        print('validating {} against {} for package platform {}'.format(args[0], platform, package_platform))
        platform_failed = validator.validate(platform)
        if platform_failed:
            print('{} FAILED validation against {} for package platform {}'.format(args[0], platform, package_platform))
            package_failed = platform_failed
        else:
            print('{} is VALIDATED against {} for package platform {}'.format(args[0], platform, package_platform))

    if validator.options.exception_file != '':
        validator.exceptions_output.close()

    return package_failed


if __name__ == '__main__':
    # pylint: disable-msg=C0103
    main_results = main()
    sys.exit(main_results)
