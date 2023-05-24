/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Crc.h>

#include "UIFrameworkAPI.h"
#include <AzToolsFramework/UI/LegacyFramework/Core/EditorFrameworkAPI.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // '...' needs to have dll-interface to be used by clients of class '...'
#include <QtCore/QObject>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTableView>
#include <QtGui/QStandardItemModel>
AZ_POP_DISABLE_WARNING
#endif

class QAction;
class QUrl;

namespace LUAEditor
{
    class MainWindow;
}

namespace AzToolsFramework
{
    class PanelRegistryInterface;
    class AZPreferencesView;
    class AZPreferencesItem;
    class AZPreferencesDataModel;
    class AZQtApplication;

    class QTickBusTicker
        : public QObject
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(QTickBusTicker, AZ::SystemAllocator);

        QTickBusTicker();
        static QTickBusTicker* SpinUp();

    private:
        QThread* m_ptrThread;
        volatile bool m_cancelled;
        volatile bool m_processing;


    public slots:
        void process();
        void cancel();

    signals:
        void finished();
        void doTick();
    };

    class Framework
        : public QObject
        , public AZ::Component
        , FrameworkMessages::Handler
        , LegacyFramework::CoreMessageBus::Handler
        , public AZ::SystemTickBus::Handler
    {
        Q_OBJECT
    public:
        friend class AZPreferencesView;
        friend class AZPreferencesItem;

        AZ_COMPONENT(Framework, "{7E1721AE-5324-40ea-9E6E-8EDBF3BC7B2C}")

        Framework();
        virtual ~Framework(void);

        void OnSystemTick();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        virtual void Init();
        virtual void Activate();
        virtual void Deactivate();
        static void Reflect(AZ::ReflectContext* context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorFramework::CoreMessageBus::Handler
        virtual void Run();
        virtual void RunAsAnotherInstance();
        virtual void OnProjectSet(const char* projectPath);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // FrameworkMessages::Handler
        // register a hotkey to make a known hotkey that can be modified by the user
        virtual void RegisterHotkey(const HotkeyDescription& desc);

        // register an action to belong to a particular registered hotkey.
        // when you do this, it will automatically change the action to use the new hotkey and also update it when it changes
        virtual void RegisterActionToHotkey(const AZ::u32 m_hotkeyID, QAction* pAction);

        // note that you don't HAVE to unregister it.  Qt sends us a message when an action is destroyed.  So just delete the action if you want.
        virtual void UnregisterActionFromHotkey(QAction* pAction);

        // the user has asked to quit.
        virtual void UserWantsToQuit();

        virtual void AddComponentInfo(MainWindowDescription& desc);
        virtual void GetComponentsInfo(AZStd::list<MainWindowDescription>& theComponents);

        virtual void PopulateApplicationMenu(QMenu* theMenu);
        virtual void RequestMainWindowClose(AZ::Uuid id);
        virtual void ApplicationCensusReply(bool isOpen);

        //////////////////////////////////////////////////////////////////////////
        // Preferences_Shell::PreferenceBus::Handler
        virtual void PreferencesOpening(QTabWidget* panel);
        virtual void PreferencesAccepted();
        virtual void PreferencesRejected();
        virtual void PreferencesRevertToDefault();
        //////////////////////////////////////////////////////////////////////////

    private:
        Framework(const Framework&) = delete;
        // utilities to make using common GUI elements across component contexts easy
        AZStd::list<MainWindowDescription> m_MainWindowList;
        int m_ApplicationCensusResults;
        QAction* m_ActionPreferences;
        QAction* m_ActionQuit;
        QAction* m_ActionChangeProject;
        AZStd::list<QAction*> m_ComponentWindowsActions;

        //////////////////////////////////////////////////////////////////////////


        virtual bool eventFilter(QObject* obj, QEvent* event);

        AZQtApplication* pApplication;

        struct HotkeyData
        {
            typedef AZStd::unordered_set<QAction*> ActionContainerType;

            HotkeyDescription m_desc;
            ActionContainerType m_actionsBound;

            HotkeyData() {}
            HotkeyData(const HotkeyDescription& desc)
                : m_desc(desc) {}
            HotkeyData(const HotkeyData& other)
                : m_desc(other.m_desc)
                , m_actionsBound(other.m_actionsBound)
            { }
            HotkeyData& operator=(const HotkeyData& other)
            {
                m_desc = other.m_desc;
                m_actionsBound = other.m_actionsBound;
                return *this;
            }

            void SelfBindActions();
        };

        typedef AZStd::unordered_map<AZ::u32, HotkeyData> HotkeyDescriptorContainerType;
        typedef AZStd::unordered_map<QAction*, AZ::u32> LiveHotkeyContainer;

        HotkeyDescriptorContainerType m_hotkeyDescriptors;
        LiveHotkeyContainer m_liveHotkeys;

        // Preferences Client
        AZPreferencesDataModel* m_Model;
        AZPreferencesView* m_View;
        void PreferencesClosing();
        void PopulateTheModel(AZPreferencesDataModel*);
        void ApplyModelToLive(AZPreferencesDataModel*);
        AZPreferencesItem* CreateHotkeyItem(HotkeyData&);

        QTickBusTicker* m_ptrTicker;

    private slots:
        void performBusTick();
        void BootStrapRemainingSystems();
        void onActionDestroyed(QObject* pObject);
        void CheckForReadyToQuit();
        void OnMenuPreferences();
        void OnMenuQuit();
        void OnShowWindowTriggered(QAction* action);
        void UserWantsToQuitProcess();


    private:
        bool m_bTicking;
    };

    class AZPreferencesView
        : public QTableView
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AZPreferencesView, AZ::SystemAllocator)

        AZPreferencesView(QWidget* parent = 0);
        virtual ~AZPreferencesView();

        virtual void CaptureHotkeyAccepted(QKeySequence keysPressed);

    private:

        QKeySequence clickResult;

    public slots:
        void OnDoubleClicked(const QModelIndex&);
    };

    class AZPreferencesItem
        : public QStandardItem
    {
    public:
        AZ_CLASS_ALLOCATOR(AZPreferencesItem, AZ::SystemAllocator)

        AZPreferencesItem(Framework::HotkeyData& hkData);
        virtual ~AZPreferencesItem();

        void SetCurrentKey(QKeySequence&);
        void ClearCurrentKey();

        Framework::HotkeyData hotkeyData;
        Framework::HotkeyData& hotkeySource;
    private:
        AZPreferencesItem(const AZPreferencesItem& other);
        AZPreferencesItem& operator=(const AZPreferencesItem&);
    };

    class AZPreferencesDataModel
        : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AZPreferencesDataModel, AZ::SystemAllocator)

        AZPreferencesDataModel(QObject* pParent);
        virtual ~AZPreferencesDataModel();

        virtual int rowCount(const QModelIndex&) const;
        virtual int columnCount(const QModelIndex&) const;
        virtual QVariant data(const QModelIndex&, int role) const;
        virtual QVariant headerData(int section, Qt::Orientation, int role) const;

        virtual Qt::ItemFlags flags(const QModelIndex& index) const;

        void AddItem(AZPreferencesItem*);
        void RevertToDefault();
        void ApplyToLive();
        AZPreferencesItem* itemAtIndex(int);

        void SafeApplyKeyToItem(QKeySequence&, AZPreferencesItem*);
    private:

        typedef AZStd::vector<AZPreferencesItem*> vecPrefs;

        vecPrefs m_Items;
    };
}
