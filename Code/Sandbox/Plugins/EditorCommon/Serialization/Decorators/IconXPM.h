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

#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_ICONXPM_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_ICONXPM_H
#pragma once

namespace Serialization {
    class IArchive;

    // Icon, stored in XPM format
    struct IconXPM
    {
        const char* const* source;
        int lineCount;

        IconXPM()
            : source(0)
            , lineCount(0)
        {
        }
        template<size_t Size>
        explicit IconXPM(const char* (&xpm)[Size])
        {
            source = xpm;
            lineCount = Size;
        }

        void Serialize([[maybe_unused]] Serialization::IArchive& ar)     {}
        bool operator<(const IconXPM& rhs) const { return source < rhs.source; }
    };

    struct IconXPMToggle
    {
        bool* variable_;
        bool value_;
        IconXPM iconTrue_;
        IconXPM iconFalse_;

        template<size_t Size1, size_t Size2>
        IconXPMToggle(bool& variable, char* (&xpmTrue)[Size1], char* (&xpmFalse)[Size2])
            : iconTrue_(xpmTrue)
            , iconFalse_(xpmFalse)
            , variable_(&variable)
            , value_(variable)
        {
        }

        IconXPMToggle(bool& variable, const IconXPM& iconTrue, const IconXPM& iconFalse)
            : iconTrue_(iconTrue)
            , iconFalse_(iconFalse)
            , variable_(&variable)
            , value_(variable)
        {
        }

        IconXPMToggle(const IconXPMToggle& orig)
            : variable_(0)
            , value_(orig.value_)
            , iconTrue_(orig.iconTrue_)
            , iconFalse_(orig.iconFalse_)
        {
        }

        IconXPMToggle()
            : variable_(0)
        {
        }
        IconXPMToggle& operator=(const IconXPMToggle& rhs)
        {
            value_ = rhs.value_;
            return *this;
        }
        ~IconXPMToggle()
        {
            if (variable_)
            {
                * variable_ = value_;
            }
        }

        template<class TArchive>
        void Serialize(TArchive& ar)
        {
            ar(value_, "value", "Value");
        }
    };
}

#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_DECORATORS_ICONXPM_H
