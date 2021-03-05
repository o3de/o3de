/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_SUFFIXUTIL_H
#define CRYINCLUDE_CRYCOMMONTOOLS_SUFFIXUTIL_H
#pragma once


// convenience class to work with suffixes in filenames, like in like dirt_ddn.dds
class SuffixUtil
{
public:

    // filename allowed to have many suffixes (e.g. "test_ddn_bump.dds" has "bump" and "ddn"
    // as suffixes (assuming that suffixSeparator is '_').
    // suffixes in file extension are also considered (e.g. "test_abc.my_data" has "abc" and "data" as suffixes)
    // suffixes in path part are also considered. if it's not what you want - remove path before calling this function.
    // comparison is case insensitive
    static bool HasSuffix(const char* const filename, const char suffixSeparator, const char* const suffix)
    {
        assert(filename);
        assert(suffix && suffix[0]);

        const size_t suffixLen = strlen(suffix);

        for (const char* p = filename; *p; ++p)
        {
            if (p[0] != suffixSeparator)
            {
                continue;
            }

            if (azmemicmp(&p[1], suffix, suffixLen) != 0)
            {
                continue;
            }

            const char c = p[1 + suffixLen];
            if ((c == 0) || (c == suffixSeparator) || (c == '.'))
            {
                return true;
            }
        }

        return false;
    }
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_SUFFIXUTIL_H
