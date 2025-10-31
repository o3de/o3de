/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #pragma once
 
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/base.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/UserSettings/UserSettings.h>

class QWidget;


namespace AzToolsFramework
{
    // the QWidgetSavedState is a nice base class for you to derive your own dialog or other widget's saved state from
    // it takes care of the geometry save and restore, using CaptureGeometry(widget) and RestoreGeometry(widget).
    class AZTF_API QWidgetSavedState : public AZ::UserSettings
    {
    public:
        AZ_RTTI(QWidgetSavedState, "{ED8EEF58-C577-4FCC-AE84-66FF63A1BDEB}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(QWidgetSavedState, AZ::SystemAllocator);
        
        virtual ~QWidgetSavedState() {}

        virtual void CaptureGeometry(QWidget *pWidget);
        virtual void RestoreGeometry(QWidget *pWidget);

        static void Reflect(AZ::ReflectContext* context);
    private:
        AZStd::vector<AZ::u8> m_windowGeometry;
    };
}
