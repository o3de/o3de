/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Outcome/Outcome.h>
#include <GraphCanvas/Types/Types.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Data/Data.h>

class QLineEdit;
class QPushButton;
class QTableView;
class QToolButton;

namespace ScriptCanvas { class Slot; }

namespace GraphCanvas
{
    struct Endpoint;

    class GraphCanvasMimeEvent;
    class GraphCanvasTreeItem;

    class NodePaletteDockWidget;
}

namespace ScriptCanvas
{
    class ScriptCanvasAssetBase;
}

namespace ScriptCanvasEditor
{
    struct CategoryInformation;
    struct NodePaletteModelInformation;

    namespace Tracker
    {
        enum class ScriptCanvasFileState : AZ::s32
        {
            NEW,
            MODIFIED,
            UNMODIFIED,
            SOURCE_REMOVED,
            INVALID = -1
        };
    }

    namespace Widget
    {
        struct GraphTabMetadata;
    }

    namespace TypeDefs
    {
        typedef AZStd::pair<AZ::EntityId, AZ::ComponentId> EntityComponentId;
    }

    class GeneralRequests
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Opens an existing graph and returns the tab index in which it was open in.
        //! \param File AssetId
        //! \return index of open tab if the asset was able to be open successfully or error message of why the open failed
        virtual AZ::Outcome<int, AZStd::string> OpenScriptCanvasAsset(SourceHandle scriptCanvasAssetId, Tracker::ScriptCanvasFileState fileState, int tabIndex = -1) = 0;
        virtual AZ::Outcome<int, AZStd::string> OpenScriptCanvasAssetId(const SourceHandle& scriptCanvasAsset, Tracker::ScriptCanvasFileState fileState) = 0;
        
        virtual int CloseScriptCanvasAsset(const SourceHandle&) = 0;

        virtual bool CreateScriptCanvasAssetFor(const TypeDefs::EntityComponentId& requestingComponent) = 0;

        virtual bool IsScriptCanvasAssetOpen(const SourceHandle& assetId) const = 0;

        virtual void OnChangeActiveGraphTab(SourceHandle) {}

        virtual void CreateNewRuntimeAsset() = 0;

        virtual ScriptCanvas::ScriptCanvasId GetActiveScriptCanvasId() const
        {
            return ScriptCanvas::ScriptCanvasId();
        }

        virtual GraphCanvas::GraphId GetActiveGraphCanvasGraphId() const
        {
            return GraphCanvas::GraphId();
        }

        virtual GraphCanvas::GraphId GetGraphCanvasGraphId([[maybe_unused]] const ScriptCanvas::ScriptCanvasId& scriptCanvasEntityId) const
        {
            return GraphCanvas::GraphId();
        }

        virtual ScriptCanvas::ScriptCanvasId GetScriptCanvasId([[maybe_unused]] const GraphCanvas::GraphId& graphCanvasSceneId) const
        {
            return ScriptCanvas::ScriptCanvasId();
        }

        virtual GraphCanvas::GraphId FindGraphCanvasGraphIdByAssetId([[maybe_unused]] const SourceHandle& assetId) const
        {
            return GraphCanvas::GraphId();
        }

        virtual ScriptCanvas::ScriptCanvasId FindScriptCanvasIdByAssetId([[maybe_unused]] const SourceHandle& assetId) const
        {
            return ScriptCanvas::ScriptCanvasId();
        }
        
        virtual bool IsInUndoRedo(const AZ::EntityId& graphCanvasGraphId) const = 0;
        virtual bool IsScriptCanvasInUndoRedo(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const = 0;

        virtual bool IsActiveGraphInUndoRedo() const = 0;

        virtual void UpdateName(const AZ::EntityId& /*graphId*/, const AZStd::string& /*name*/) {}

        virtual void DeleteNodes(const AZ::EntityId& /*sceneId*/, const AZStd::vector<AZ::EntityId>& /*nodes*/){}
        virtual void DeleteConnections(const AZ::EntityId& /*sceneId*/, const AZStd::vector<AZ::EntityId>& /*connections*/){}

        virtual void DisconnectEndpoints(const AZ::EntityId& /*sceneId*/, const AZStd::vector<GraphCanvas::Endpoint>& /*endpoints*/) {}

        virtual void PostUndoPoint(ScriptCanvas::ScriptCanvasId) = 0;
        virtual void SignalSceneDirty(SourceHandle) = 0;

        // Increment the value of the ignore undo point tracker
        virtual void PushPreventUndoStateUpdate() = 0;
        // Decrement the value of the ignore undo point tracker
        virtual void PopPreventUndoStateUpdate() = 0;
        // Sets the value of the ignore undo point tracker to 0.
        // Therefore allowing undo points to be posted
        virtual void ClearPreventUndoStateUpdate() = 0;

        virtual void TriggerUndo() = 0;
        virtual void TriggerRedo() = 0;

        virtual const CategoryInformation* FindNodePaletteCategoryInformation(AZStd::string_view categoryPath) const = 0;
        virtual const NodePaletteModelInformation* FindNodePaletteModelInformation(const ScriptCanvas::NodeTypeIdentifier& nodeType) const = 0;
    };

    using GeneralRequestBus = AZ::EBus<GeneralRequests>;

    class GeneralEditorNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvas::ScriptCanvasId;

        virtual void OnUndoRedoBegin() {}
        virtual void OnUndoRedoEnd() {}
    };

    using GeneralEditorNotificationBus = AZ::EBus<GeneralEditorNotifications>;

    class GeneralAssetNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = SourceHandle;

        virtual void OnAssetVisualized() {};
        virtual void OnAssetUnloaded() {};
    };

    using GeneralAssetNotificationBus = AZ::EBus<GeneralAssetNotifications>;

    class NodeCreationNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvas::ScriptCanvasId;

        virtual void OnGraphCanvasNodeCreated(const AZ::EntityId& nodeId) = 0;
    };

    using NodeCreationNotificationBus = AZ::EBus<NodeCreationNotifications>;

    class VariablePaletteRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void RegisterVariableType(const ScriptCanvas::Data::Type& variabletype) = 0;

        virtual bool IsValidVariableType(const ScriptCanvas::Data::Type& variableType) const = 0;

        struct SlotSetup
        {
            AZStd::string m_name;
            AZ::Uuid m_type = AZ::Uuid::CreateNull();
        };

        virtual bool ShowSlotTypeSelector(ScriptCanvas::Slot* slot, const QPoint& scenePosition, SlotSetup&) = 0;
    };

    using VariablePaletteRequestBus = AZ::EBus<VariablePaletteRequests>;

    class VariableAutomationRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual AZStd::vector< ScriptCanvas::Data::Type > GetPrimitiveTypes() const = 0;
        virtual AZStd::vector< ScriptCanvas::Data::Type > GetBehaviorContextObjectTypes() const = 0;
        virtual AZStd::vector< ScriptCanvas::Data::Type > GetMapTypes() const = 0;
        virtual AZStd::vector< ScriptCanvas::Data::Type > GetArrayTypes() const = 0;

        AZStd::vector< ScriptCanvas::Data::Type > GetVariableTypes() const
        {
            AZStd::vector< ScriptCanvas::Data::Type > dataTypes = GetPrimitiveTypes();

            auto workingTypes = GetBehaviorContextObjectTypes();
            dataTypes.insert(dataTypes.begin(), workingTypes.begin(), workingTypes.end());
            workingTypes.clear();

            workingTypes = GetMapTypes();
            dataTypes.insert(dataTypes.begin(), workingTypes.begin(), workingTypes.end());
            workingTypes.clear();

            workingTypes = GetArrayTypes();
            dataTypes.insert(dataTypes.begin(), workingTypes.begin(), workingTypes.end());
            workingTypes.clear();

            return dataTypes;
        }

        virtual bool IsShowingVariablePalette() const = 0;
        virtual bool IsShowingGraphVariables() const = 0;

        virtual QPushButton* GetCreateVariableButton() const = 0;
        virtual QTableView* GetGraphPaletteTableView() const = 0;
        virtual QTableView* GetVariablePaletteTableView() const = 0;

        virtual QLineEdit* GetVariablePaletteFilter() const = 0;
        virtual QLineEdit* GetGraphVariablesFilter() const = 0;
    };

    using VariableAutomationRequestBus = AZ::EBus<VariableAutomationRequests>;

    class AutomationRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual NodeIdPair ProcessCreateNodeMimeEvent(GraphCanvas::GraphCanvasMimeEvent* mimeEvent, const AZ::EntityId& graphCanvasGraphId, AZ::Vector2 nodeCreationPos) = 0;
        virtual const GraphCanvas::GraphCanvasTreeItem* GetNodePaletteRoot() const = 0;

        virtual void SignalAutomationBegin() = 0;
        virtual void SignalAutomationEnd() = 0;

        virtual void ForceCloseActiveAsset() = 0;
    };

    using AutomationRequestBus = AZ::EBus<AutomationRequests>;
}
