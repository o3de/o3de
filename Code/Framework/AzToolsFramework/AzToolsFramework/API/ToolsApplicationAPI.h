/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
//AZTF-SHARED
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/base.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzFramework/Entity/EntityContextBus.h>

#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/API/ToolsApplicationBus.h>
#include <QObject>

namespace AZ
{
    class Entity;
    class Vector2;
} // namespace AZ

struct IEditor;
class QApplication;
class QDockWidget;
class QMainWindow;
class QMenu;
class QString;
class QWidget;

namespace AzToolsFramework
{
    struct ViewPaneOptions;
    class EntityPropertyEditor;

    namespace UndoSystem
    {
        class UndoStack;
        class URSequencePoint;
    }

    namespace AssetBrowser
    {
        class AssetSelectionModel;
    }

    using ClassDataList = AZStd::vector<const AZ::SerializeContext::ClassData*>;

    //! Return true to accept this type of component.
    using ComponentFilter = AZStd::function<bool(const AZ::SerializeContext::ClassData&)>;

    /**
     * RAII Helper class for undo batches.
     *
     * AzToolsFramework::ScopedUndoBatch undoBatch("Batch Name");
     * entity->ChangeData(...);
     * undoBatch.MarkEntityDirty(entity->GetId());
     */
    class AZTF_API ScopedUndoBatch
    {
    public:
        AZ_CLASS_ALLOCATOR(ScopedUndoBatch, AZ::SystemAllocator);
        explicit ScopedUndoBatch(const char* batchName);
        ~ScopedUndoBatch();

        // utility/convenience function for adding dirty entity
        static void MarkEntityDirty(const AZ::EntityId& id);
        UndoSystem::URSequencePoint* GetUndoBatch() const;

    private:
        AZ_DISABLE_COPY_MOVE(ScopedUndoBatch);

        UndoSystem::URSequencePoint* m_undoBatch = nullptr;
    };

    /// Registers a view pane with the main editor. It will be listed under the "Tools" menu on the main window's menubar.
    /// Note that if a view pane is registered with it's ViewPaneOptions.isDeletable set to true, the widget will be deallocated and destructed on close.
    /// Otherwise, it will be hidden instead.
    /// If you'd like to be able to veto the close (for instance, if the user has unsaved data), override the closeEvent() on your custom
    /// viewPane widget and call ignore() on the QCloseEvent* parameter.
    ///
    /// \param viewPaneName - name for the pane. This is what will appear in the dock window's title bar, as well as in the main editor window's menubar, if the optionalMenuText is not set in the viewOptions parameter.
    /// \param category - category under the "Tools" menu that will contain the option to open the newly registered pane.
    /// \param viewOptions - structure defining various options for the pane.
    template <typename TWidget>
    inline void RegisterViewPane(const char* name, const char* category, const ViewPaneOptions& viewOptions)
    {
        AZStd::function<QWidget*(QWidget*)> windowCreationFunc = [](QWidget* parent = nullptr)
        {
            return new TWidget(parent);
        };

        EditorRequests::Bus::Broadcast(&EditorRequests::RegisterViewPane, name, category, viewOptions, windowCreationFunc);
    }

    /// Registers a view pane with the main editor. It will be listed under the "Tools" menu on the main window's menubar.
    /// This variant is most useful when dealing with singleton view widgets.
    /// Note that if the new view is a singleton, and shouldn't be destroyed by the view pane manager, viewOptions.isDeletable must be set to false.
    /// Note that if a view pane is registered with it's ViewPaneOptions.isDeletable set to true, the widget will be deallocated and destructed on close.
    /// Otherwise, it will be hidden instead.
    /// If you'd like to be able to veto the close (for instance, if the user has unsaved data), override the closeEvent() on your custom
    /// viewPane widget and call ignore() on the QCloseEvent* parameter.
    ///
    /// \param viewPaneName - name for the pane. This is what will appear in the dock window's title bar, as well as in the main editor window's menubar, if the optionalMenuText is not set in the viewOptions parameter.
    /// \param category - category under the "Tools" menu that will contain the option to open the newly registered pane.
    /// \param viewOptions - structure defining various options for the pane.
    template <typename TWidget>
    inline void RegisterViewPane(const char* viewPaneName, const char* category, const ViewPaneOptions& viewOptions, AZStd::function<QWidget*(QWidget*)> windowCreationFunc)
    {
        EditorRequests::Bus::Broadcast(&EditorRequests::RegisterViewPane, viewPaneName, category, viewOptions, windowCreationFunc);
    }

    /// Unregisters a view pane with the main editor. It will no longer be listed under the "Tools" menu on the main window's menubar.
    /// Any currently open view panes of this type will be closed before the view pane handlers are unregistered.
    ///
    /// \param viewPaneName - name of the pane to unregister. Must be the same as the name previously registered with RegisterViewPane.
    AZTF_API void UnregisterViewPane(const char* viewPaneName);

    /// Returns the widget contained/wrapped in a view pane.
    /// \param name - the name of the pane which contains the widget to be retrieved. This must match the name used for registration.
    template <typename TWidget>
    inline TWidget* GetViewPaneWidget(const char* viewPaneName)
    {
        QWidget* ret = nullptr;
        EditorRequests::Bus::BroadcastResult(ret, &EditorRequests::GetViewPaneWidget, viewPaneName);

        return qobject_cast<TWidget*>(ret);
    }

    /// Opens a view pane if not already open, and activating the view pane if it was already opened.
    ///
    /// \param viewPaneName - name of the pane to open/activate. Must be the same as the name previously registered with RegisterViewPane.
    AZTF_API void OpenViewPane(const char* viewPaneName);

    /// Opens a view pane if not already open, and activating the view pane if it was already opened.
    ///
    /// \param viewPaneName - name of the pane to open/activate. Must be the same as the name previously registered with RegisterViewPane.
    AZTF_API QDockWidget* InstanceViewPane(const char* viewPaneName);

    /// Closes a view pane if it is currently open.
    ///
    /// \param viewPaneName - name of the pane to open/activate. Must be the same as the name previously registered with RegisterViewPane.
    AZTF_API void CloseViewPane(const char* viewPaneName);

    /**
     * Helper to wrap checking if an undo/redo operation is in progress.
     */
    AZTF_API bool UndoRedoOperationInProgress();
} // namespace AzToolsFramework

AZ_DECLARE_BUDGET_SHARED(AzToolsFramework);
