/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <QCoreApplication>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>

#include "GeneralNodePaletteTreeItemTypes.h"

#include "Editor/Components/IconComponent.h"

#include "Editor/Nodes/NodeCreateUtils.h"
#include "Editor/Translation/TranslationHelper.h"

#include "ScriptCanvas/Bus/RequestBus.h"
#include "Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h"
#include "Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h"

#include <Core/Attributes.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>
#include <Libraries/Core/Method.h>
#include <Libraries/Core/MethodOverloaded.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////////
    // CreateClassMethodMimeEvent
    ///////////////////////////////

    void CreateClassMethodMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateClassMethodMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(1)
                ->Field("ClassName", &CreateClassMethodMimeEvent::m_className)
                ->Field("MethodName", &CreateClassMethodMimeEvent::m_methodName)
                ->Field("IsOverload", &CreateClassMethodMimeEvent::m_isOverload)
                ->Field("propertyStatus", &CreateClassMethodMimeEvent::m_propertyStatus)
                ;
        }
    }

    CreateClassMethodMimeEvent::CreateClassMethodMimeEvent(const QString& className, const QString& methodName, bool isOverload, ScriptCanvas::PropertyStatus propertyStatus)
        : m_className(className.toUtf8().data())
        , m_methodName(methodName.toUtf8().data())
        , m_isOverload(isOverload)
        , m_propertyStatus(propertyStatus)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateClassMethodMimeEvent::CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const
    {
        if (m_isOverload)
        {
            return Nodes::CreateObjectMethodOverloadNode(m_className, m_methodName, scriptCanvasId);
        }
        else
        {
            return Nodes::CreateObjectMethodNode(m_className, m_methodName, scriptCanvasId, m_propertyStatus);
        }
    }

    ////////////////////////////////////
    // ClassMethodEventPaletteTreeItem
    ////////////////////////////////////

    ClassMethodEventPaletteTreeItem::ClassMethodEventPaletteTreeItem(AZStd::string_view className, AZStd::string_view methodName, bool isOverload, ScriptCanvas::PropertyStatus propertyStatus)
        : DraggableNodePaletteTreeItem(methodName, ScriptCanvasEditor::AssetEditorId)
        , m_className(className.data())
        , m_methodName(methodName.data())
        , m_isOverload(isOverload)
        , m_propertyStatus(propertyStatus)
    {
        GraphCanvas::TranslationKey key;


        AZStd::string updatedMethodName;
        if (propertyStatus != ScriptCanvas::PropertyStatus::None)
        {
            updatedMethodName = (propertyStatus == ScriptCanvas::PropertyStatus::Getter) ? "Get" : "Set";
        }
        updatedMethodName.append(methodName);

        key << "BehaviorClass" << className << "methods" << updatedMethodName << "details";

        GraphCanvas::TranslationRequests::Details details;
        details.m_name = methodName;
        details.m_subtitle = className;

        GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

        SetName(details.m_name.c_str());
        SetToolTip(details.m_tooltip.c_str());

        SetTitlePalette("MethodNodeTitlePalette");
    }

    GraphCanvas::GraphCanvasMimeEvent* ClassMethodEventPaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateClassMethodMimeEvent(m_className, m_methodName, m_isOverload, m_propertyStatus);
    }

    AZStd::string ClassMethodEventPaletteTreeItem::GetClassMethodName() const
    {
        return m_className.toUtf8().data();
    }

    AZStd::string ClassMethodEventPaletteTreeItem::GetMethodName() const
    {
        return m_methodName.toUtf8().data();
    }

    bool ClassMethodEventPaletteTreeItem::IsOverload() const
    {
        return m_isOverload;
    }

    ScriptCanvas::PropertyStatus ClassMethodEventPaletteTreeItem::GetPropertyStatus() const
    {
        return m_propertyStatus;
    }

    //! Implementation of the CreateGlobalMethod Mime Event
    void CreateGlobalMethodMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateGlobalMethodMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("MethodName", &CreateGlobalMethodMimeEvent::m_methodName)
                ;
        }
    }

    CreateGlobalMethodMimeEvent::CreateGlobalMethodMimeEvent(AZStd::string methodName)
        : m_methodName{ AZStd::move(methodName) }
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateGlobalMethodMimeEvent::CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const
    {
        return Nodes::CreateGlobalMethodNode(m_methodName, scriptCanvasId);
    }

    //! Global Method Palette Tree Item implementation
    GlobalMethodEventPaletteTreeItem::GlobalMethodEventPaletteTreeItem(const GlobalMethodNodeModelInformation& nodeModelInformation)
        : DraggableNodePaletteTreeItem(nodeModelInformation.m_methodName, ScriptCanvasEditor::AssetEditorId)
        , m_methodName{ nodeModelInformation.m_methodName }
    {
        SetToolTip(QString::fromUtf8(nodeModelInformation.m_toolTip.data(),
            aznumeric_cast<int>(nodeModelInformation.m_toolTip.size())));

        SetTitlePalette("MethodNodeTitlePalette");
        if (!nodeModelInformation.m_displayName.empty())
        {
            SetName(nodeModelInformation.m_displayName.c_str());
        }
    }

    GraphCanvas::GraphCanvasMimeEvent* GlobalMethodEventPaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateGlobalMethodMimeEvent(m_methodName);
    }


    const AZStd::string& GlobalMethodEventPaletteTreeItem::GetMethodName() const
    {
        return m_methodName;
    }

    AZ::IO::Path GlobalMethodEventPaletteTreeItem::GetTranslationDataPath() const
    {
        AZStd::string propertyName = m_methodName;
        AZ::StringFunc::Replace(propertyName, "::Getter", "");
        AZ::StringFunc::Replace(propertyName, "::Setter", "");

        AZStd::string filename = GraphCanvas::TranslationKey::Sanitize(propertyName);

        return AZ::IO::Path("Properties") / filename;
    }

    void GlobalMethodEventPaletteTreeItem::GenerateTranslationData()
    {
        AZStd::string propertyName = m_methodName;
        AZ::StringFunc::Replace(propertyName, "::Getter", "");
        AZ::StringFunc::Replace(propertyName, "::Setter", "");

        ScriptCanvasEditorTools::TranslationGeneration translation;
        translation.TranslateBehaviorProperty(propertyName);
    }

    //////////////////////////////
    // CreateCustomNodeMimeEvent
    //////////////////////////////

    void CreateCustomNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateCustomNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(2)
                ->Field("TypeId", &CreateCustomNodeMimeEvent::m_typeId)
                ->Field("StyleOverride", &CreateCustomNodeMimeEvent::m_styleOverride)
                ->Field("TitlePalette", &CreateCustomNodeMimeEvent::m_titlePalette)
                ;
        }
    }

    CreateCustomNodeMimeEvent::CreateCustomNodeMimeEvent(const AZ::Uuid& typeId)
        : m_typeId(typeId)
    {
    }

    CreateCustomNodeMimeEvent::CreateCustomNodeMimeEvent(const AZ::Uuid& typeId, const AZStd::string& styleOverride, const AZStd::string& titlePalette)
        : m_typeId(typeId)
        , m_styleOverride(styleOverride)
        , m_titlePalette(titlePalette)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateCustomNodeMimeEvent::CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const
    {
        Nodes::StyleConfiguration styleConfiguration;
        styleConfiguration.m_nodeSubStyle = m_styleOverride;
        styleConfiguration.m_titlePalette = m_titlePalette;

        return Nodes::CreateNode(m_typeId, scriptCanvasId, styleConfiguration);
    }

    //////////////////////////////
    // CustomNodePaletteTreeItem
    //////////////////////////////

    CustomNodePaletteTreeItem::CustomNodePaletteTreeItem(const ScriptCanvasEditor::CustomNodeModelInformation& info)
        : DraggableNodePaletteTreeItem(info.m_displayName, ScriptCanvasEditor::AssetEditorId)
        , m_info(info)
        , m_typeId(info.m_typeId)
    {
    }

    GraphCanvas::GraphCanvasMimeEvent* CustomNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateCustomNodeMimeEvent(m_typeId, GetStyleOverride(), GetTitlePalette());
    }

    AZ::Uuid CustomNodePaletteTreeItem::GetTypeId() const
    {
        return m_typeId;
    }
}
