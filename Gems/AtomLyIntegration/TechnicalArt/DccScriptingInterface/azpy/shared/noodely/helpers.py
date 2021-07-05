"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

###########################################################################
# HELPER method functions
# -------------------------------------------------------------------------


def istext(filename):
    """
    A guess if a file is text or binary
    """
    s = open(filename).read(512)
    text_characters = "".join(map(chr, range(32, 127)) + list("\n\r\t\b"))
    _null_trans = string.maketrans("", "")
    if not s:
        # Empty files are considered text
        return True
    if "\0" in s:
        # Files with null bytes are likely binary
        return False
    # Get the non-text characters (maps a character to itself then
    # use the 'remove' option to get rid of the text characters.)
    t = s.translate(_null_trans, text_characters)
    # If more than 30% non-text characters, then
    # this is considered a binary file
    if float(len(t)) / float(len(s)) > 0.30:
        return False
    return True


def display_cached_value(cache, cache_key):
    try:
        cached_value = cache[cache_key]
        print("{0}={1}".format(cache_key, cached_value))
    except KeyError:
        print("{0}=Not in cache".format(cache_key))
