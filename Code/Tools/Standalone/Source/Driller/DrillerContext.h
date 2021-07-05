/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzToolsFramework/UI/LegacyFramework/Core/EditorFrameworkAPI.h>
#include "DrillerContextInterface.h"
#include "DrillerMainWindow.hxx"

#include <QStandardItem>

#pragma once


namespace Driller
{
    //////////////////////////////////////////////////////////////////////////
    // Context
    // all editor components are responsible for maintaining the list of documents they are responsible for
    // and setting up editing facilities on those asset types in that "space".
    // editor contexts are components and have component ID's because we will communicate to them via buses.

    // this is the data side of drilling.
    // for example, data flow, discovery, that sort of thing.

    class Context
        : public AZ::Component
        , private LegacyFramework::CoreMessageBus::Handler
        , private ContextInterface::Handler
    {
        friend class ContextFactory;
    public:
        AZ_COMPONENT(Driller::Context, "{60EC92BD-1D96-4E37-AB46-DF89A5497617}")

        Context();
        virtual ~Context();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        virtual void Init();
        virtual void Activate();
        virtual void Deactivate();
        static void Reflect(AZ::ReflectContext* context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorFramework CoreMessages
        virtual void OnRestoreState(); // sent when everything is registered up and ready to go, this is what bootstraps stuff to get going.
        virtual bool OnGetPermissionToShutDown();
        virtual bool CheckOkayToShutDown();
        virtual void OnSaveState(); // sent to everything when the app is about to shut down - do what you need to do.
        virtual void OnDestroyState();
        virtual void ApplicationDeactivated();
        virtual void ApplicationActivated();
        virtual void ApplicationShow(AZ::Uuid id);
        virtual void ApplicationHide(AZ::Uuid id);
        virtual void ApplicationCensus();
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorFramework::AssetManagementMessages
        //virtual bool RawFileOpenRequested(const EditorFramework::RegisteredAssetType& registeredTypeID, const char *fullPathName);
        //////////////////////////////////////////////////////////////////////////

        virtual void ShowDrillerView();

        //////////////////////////////////////////////////////////////////////////
        // internal data structure for the class/member/property reference panel
        // this is what we serialize and work with
        DrillerMainWindow* m_pDrillerMainWindow;

    private:

        // utility
        void ProvisionalShowAndFocus(bool forceShow = false, bool forceHide = false);
    };

    class ClassReferenceItem
        : public QStandardItem
    {
    public:
        AZ_CLASS_ALLOCATOR(ClassReferenceItem, AZ::SystemAllocator, 0);

        ClassReferenceItem(const QIcon& icon, const QString& text, size_t id);
        ClassReferenceItem(const QString& text, size_t id);
        ~ClassReferenceItem() {}

        size_t GetTypeID() {return m_ID; }
    protected:
        size_t m_ID;
    };
};

