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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_UI_TOGGLEBUTTON_H
#define CRYINCLUDE_CRYCOMMONTOOLS_UI_TOGGLEBUTTON_H
#pragma once


#include "IUIComponent.h"
#include <string>

class ToggleButton
    : public IUIComponent
{
public:
    template <typename T>
    ToggleButton(const TCHAR* text, T* object, void (T::* method)(bool value));
    ~ToggleButton();

    void SetState(bool value);

    // IUIComponent
    virtual void CreateUI(void* window, int left, int top, int width, int height);
    virtual void Resize(void* window, int left, int top, int width, int height);
    virtual void DestroyUI(void* window);
    virtual void GetExtremeDimensions(void* window, int& minWidth, int& maxWidth, int& minHeight, int& maxHeight);

private:
    ToggleButton(const ToggleButton&);
    ToggleButton& operator=(const ToggleButton);

    struct ICallback
    {
        virtual void Release() = 0;
        virtual void Call(bool value) = 0;
    };

    template <typename T>
    struct Callback
        : public ICallback
    {
        Callback(T* object, void (T::* method)(bool value))
            : object(object)
            , method(method) {}
        virtual void Release() {delete this; }
        virtual void Call(bool value) {(object->*method)(value); }
        T* object;
        void (T::* method)(bool value);
    };

    void OnChecked(bool checked);

    tstring m_text;
    void* m_button;
    void* m_font;
    bool m_state;
    ICallback* m_callback;
};

template <typename T>
ToggleButton::ToggleButton(const TCHAR* text, T* object, void (T::* method)(bool value))
    : m_text(text)
    , m_state(false)
    , m_callback(new Callback<T>(object, method))
{
}

#endif // CRYINCLUDE_CRYCOMMONTOOLS_UI_TOGGLEBUTTON_H
