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

#pragma once

#include <Serialization/IArchive.h>
#include <Serialization/Decorators/ActionButton.h>
#include <AzCore/std/functional.h>

namespace Serialization
{
    typedef AZStd::function<void()> StdFunctionActionButtonCalback;

    struct StdFunctionActionButton
        : public IActionButton
    {
        StdFunctionActionButtonCalback callback;
        string icon;

        explicit StdFunctionActionButton(const StdFunctionActionButtonCalback& callback, const char* icon = "")
            : callback(callback)
            , icon(icon)
        {
        }

        // IActionButton

        virtual void Callback() const override
        {
            if (callback)
            {
                callback();
            }
        }

        virtual const char* Icon() const override
        {
            return icon.c_str();
        }

        virtual IActionButtonPtr Clone() const override
        {
            return IActionButtonPtr(new StdFunctionActionButton(callback, icon.c_str()));
        }

        // ~IActionButton
    };

    inline bool Serialize(Serialization::IArchive& ar, StdFunctionActionButton& button, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            return ar(Serialization::SStruct::ForEdit(static_cast<Serialization::IActionButton&>(button)), name, label);
        }
        else
        {
            return false;
        }
    }

    inline StdFunctionActionButton ActionButton(const StdFunctionActionButtonCalback& callback, const char* icon = "")
    {
        return StdFunctionActionButton(callback, icon);
    }
}

