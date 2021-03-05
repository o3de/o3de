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

#include <SmartPointersHelpers.h>
#include <Serialization/IArchive.h>
#include <functor.h>

namespace Serialization
{
    struct IActionButton;

    DECLARE_SMART_POINTERS(IActionButton)

    struct IActionButton
    {
        virtual ~IActionButton() {}

        virtual void Callback() const = 0;
        virtual const char* Icon() const = 0;
        virtual IActionButtonPtr Clone() const = 0;
    };

    typedef Functor0 FunctorActionButtonCallback;

    struct FunctorActionButton
        : public IActionButton
    {
        FunctorActionButtonCallback callback;
        string icon;

        explicit FunctorActionButton(const FunctorActionButtonCallback& callback, const char* icon = "")
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
            return IActionButtonPtr(new FunctorActionButton(callback, icon.c_str()));
        }

        // ~IActionButton
    };

    inline bool Serialize(Serialization::IArchive& ar, FunctorActionButton& button, const char* name, const char* label)
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

    inline FunctorActionButton ActionButton(const FunctorActionButtonCallback& callback, const char* icon = "")
    {
        return FunctorActionButton(callback, icon);
    }
}

