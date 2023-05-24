/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Tests/SystemComponentFixture.h>

#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSystemComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <AzQtComponents/Components/DockBarButton.h>

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <Integration/AnimationBus.h>

#include <QModelIndex>
#include <QString>
#include <QToolBar>
#include <QTreeView>
#include <QTreeWidget>
#include <Tests/AssetSystemMocks.h>

QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QTreeView)
QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)
QT_FORWARD_DECLARE_CLASS(ReselectingTreeView)

namespace AzToolsFramework { class ReflectedPropertyEditor; }
namespace AzQtComponents { class WindowDecorationWrapper; }
namespace AzQtComponents { class TitleBar; }
namespace GraphCanvas { class NodePaletteTreeView; }

namespace EMotionFX
{
    class SimulatedObjectColliderWidget;
    class SkeletonOutlinerPlugin;
    class SimulatedObjectWidget;

    class MakeQtApplicationBase
    {
    public:
        MakeQtApplicationBase() = default;
        AZ_DEFAULT_COPY_MOVE(MakeQtApplicationBase);
        virtual ~MakeQtApplicationBase();

        void SetUp();

    protected:
        QApplication* m_uiApp = nullptr;
    private:
        static inline int s_argc{0};
    };

    using UIFixtureBase = ComponentFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        AZ::UserSettingsComponent,
        Physics::MaterialSystemComponent,
        AzToolsFramework::Components::PropertyManagerComponent,
        EMotionFX::Integration::SystemComponent
    >;

    // MakeQtApplicationBase is listed as the first base class, so that the
    // QApplication object is destroyed after the EMotionFX SystemComponent is
    // shut down
    class UIFixture
        : public MakeQtApplicationBase
        , public UIFixtureBase
        , private Integration::SystemNotificationBus::Handler
    {
    public:
        void SetUp() override;
        void TearDown() override;
        static QWidget* FindTopLevelWidget(const QString& objectName);
        static QWidget* GetWidgetFromToolbar(const QToolBar* toolbar, const QString &widgetText);
        static QWidget* GetWidgetFromToolbarWithObjectName(const QToolBar* toolbar, const QString &objectName);
        static QWidget* GetWidgetWithNameFromNamedToolbar(const QWidget* widget, const QString &toolBarName, const QString &objectName);

        template<class T>
        T* GetFirstChildOfType(const QWidget* widget)
        {
            const QList<T*> children = widget->findChildren<T*>();
            return children.isEmpty() ? nullptr : children[0];
        }

        static QAction* GetNamedAction(const QWidget* widget, const QString& actionName);
        QModelIndex GetIndexFromName(const GraphCanvas::NodePaletteTreeView* tree, const QString& name);
        static bool GetActionFromContextMenu(QAction*& action, const QMenu* contextMenu, const QString& actionName);

        static void ExecuteCommands(std::vector<std::string> commands);

        void CloseAllPlugins();
        void DeselectAllAnimGraphNodes();
        void CloseAllNotificationWindows();
        static void BringUpContextMenu(QObject* widget, const QPoint& pos, const QPoint& globalPos);
        void BringUpContextMenu(const QTreeView* treeview, const QRect& rect);
        void BringUpContextMenu(const QTreeWidget* treeWidget, const QRect& rect);
        void SelectIndexes(const QModelIndexList& indexList, QTreeView* treeView, const int start, const int end);

        AzToolsFramework::PropertyRowWidget* GetNamedPropertyRowWidgetFromReflectedPropertyEditor(AzToolsFramework::ReflectedPropertyEditor* rpe, const QString& name);
        void TriggerContextMenuAction(QWidget* widget, const QString& actionname);
        void TriggerModalContextMenuAction(QWidget* widget, const QString& actionname);

        AzQtComponents::WindowDecorationWrapper* GetDecorationWrapperForMainWindow() const;
        AzQtComponents::TitleBar* GetTitleBarForMainWindow() const;
        AzQtComponents::DockBarButton* GetDockBarButtonForMainWindow(AzQtComponents::DockBarButton::WindowDecorationButton buttonType) const;

        void SelectActor(Actor* actor);
        void SelectActorInstance(ActorInstance* actorInstance);

        static EMStudio::MotionSetsWindowPlugin* GetMotionSetsWindowPlugin();
        static EMStudio::MotionSetManagementWindow* GetMotionSetManagementWindow();

        void CreateAnimGraphParameter(const AZStd::string& name);

        SimulatedObjectColliderWidget* GetSimulatedObjectColliderWidget() const;
    protected:
        virtual bool ShouldReflectPhysicSystem() { return false; }
        virtual void ReflectMockedSystems();

        void OnRegisterPlugin();
        void SetupQtAndFixtureBase();
        void SetupPluginWindows();

        QApplication* m_uiApp = nullptr;
        EMStudio::AnimGraphPlugin* m_animGraphPlugin = nullptr;
        EMotionFX::SkeletonOutlinerPlugin* m_skeletonOutlinerPlugin = nullptr;
        EMotionFX::SimulatedObjectWidget* m_simulatedObjectPlugin = nullptr;

        testing::NiceMock<UnitTests::MockAssetSystemRequest> m_assetSystemRequestMock;
    };
} // end namespace EMotionFX
