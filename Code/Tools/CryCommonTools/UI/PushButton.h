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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_UI_PUSHBUTTON_H
#define CRYINCLUDE_CRYCOMMONTOOLS_UI_PUSHBUTTON_H
#pragma once


#include "IUIComponent.h"
#include <string>

class PushButton
    : public IUIComponent
{
public:
    template <typename T>
    PushButton(const TCHAR* text, T* object, void (T::* method)());
    ~PushButton();

    void Enable(bool enabled);

    // IUIComponent
    virtual void CreateUI(void* window, int left, int top, int width, int height);
    virtual void Resize(void* window, int left, int top, int width, int height);
    virtual void DestroyUI(void* window);
    virtual void GetExtremeDimensions(void* window, int& minWidth, int& maxWidth, int& minHeight, int& maxHeight);

private:
    PushButton(const PushButton&);
    PushButton& operator=(const PushButton&);

    struct ICallback
    {
        virtual void Release() = 0;
        virtual void Call() = 0;
    };

    template <typename T>
    struct Callback
        : public ICallback
    {
        Callback(T* object, void (T::* method)())
            : object(object)
            , method(method) {}
        virtual void Release() {delete this; }
        virtual void Call() {(object->*method)(); }
        T* object;
        void (T::* method)();
    };

    void OnPushed();

    std::basic_string<TCHAR> m_text;
    void* m_button;
    void* m_font;
    ICallback* m_callback;
    bool m_enabled;
};

template <typename T>
PushButton::PushButton(const TCHAR* text, T* object, void (T::* method)())
    : m_text(text)
    , m_callback(new Callback<T>(object, method))
    , m_enabled(true)
{
}

#endif // CRYINCLUDE_CRYCOMMONTOOLS_UI_PUSHBUTTON_H
