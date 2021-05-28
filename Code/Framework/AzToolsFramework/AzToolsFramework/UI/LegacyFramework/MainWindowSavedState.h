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

#ifndef MAINWINDOWSAVEDSTATE_H
#define MAINWINDOWSAVEDSTATE_H

#include <AzCore/base.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/UserSettings/UserSettings.h>

#pragma once

class QByteArray;

namespace AZ { class ReflectContext; }

namespace AzToolsFramework
{
    // the Main Window Saved State is a nice base class for you to derive your own Main Window state from
    // (or just use your own in addition).
    // it saves both the geometry and state of QMainWindows (so all splitter information, positioning, dock visibility, dock placement, etc)
    class MainWindowSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(MainWindowSavedState, "{0892EAAA-8440-4409-9E8B-35BEF203E8E1}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(MainWindowSavedState, AZ::SystemAllocator, 0);

        // the dh object stream cannot store more than 4k and sometimes the window state is about 5k.
        AZStd::vector< AZStd::vector<AZ::u8> > m_serializableWindowState;

        const AZStd::vector<AZ::u8>&  GetWindowState();
        AZStd::vector<AZ::u8> m_windowGeometry;

        MainWindowSavedState() {}
        virtual void Init(const QByteArray& windowState, const QByteArray& windowGeom);

        static void Reflect(AZ::ReflectContext* context);
    private:
        AZStd::vector<AZ::u8> m_windowState;
    };
}

#endif
