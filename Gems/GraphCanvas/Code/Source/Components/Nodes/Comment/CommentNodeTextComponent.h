/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsWidget>
#include <QPlainTextEdit>
#include <QTimer>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/TypeInfo.h>

#include <Components/Nodes/General/GeneralNodeLayoutComponent.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <Widgets/GraphCanvasLabel.h>
#include <Widgets/NodePropertyDisplayWidget.h>

class QGraphicsGridLayout;

namespace GraphCanvas
{
    class CommentTextGraphicsWidget;

    class CommentNodeTextComponent
        : public GraphCanvasPropertyComponent
        , public NodeNotificationBus::Handler
        , public CommentRequestBus::Handler
        , public CommentLayoutRequestBus::Handler
        , public EntitySaveDataRequestBus::Handler
        , public CommentNodeTextSaveDataInterface
    {
    public:
        AZ_COMPONENT(CommentNodeTextComponent, "{15C568B0-425C-4655-814D-0A299341F757}", GraphCanvasPropertyComponent);

        static void Reflect(AZ::ReflectContext*);

        CommentNodeTextComponent();
        CommentNodeTextComponent(AZStd::string_view initialText);
        ~CommentNodeTextComponent() = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GraphCanvas_CommentTextService", 0xb650db99));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incombatible)
        {
            incombatible.push_back(AZ_CRC("GraphCanvas_CommentTextService", 0xb650db99));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_StyledGraphicItemService", 0xeae4cdf4));
            required.push_back(AZ_CRC("GraphCanvas_SceneMemberService", 0xe9759a2d));
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // NodeNotification
        void OnAddedToScene(const AZ::EntityId&) override;
        ////

        // CommentRequestBus
        void SetComment(const AZStd::string& comment) override;
        const AZStd::string& GetComment() const override;

        void SetCommentMode(CommentMode commentMode) override;

        void SetBackgroundColor(const AZ::Color& color) override;
        AZ::Color GetBackgroundColor() const override;
        ////

        // CommentNodeTextSaveDataInterface
        CommentMode GetCommentMode() const override;
        ////

        // CommentLayoutRequestBus
        QGraphicsLayoutItem* GetGraphicsLayoutItem() override;
        ////

        // EntitySaveDataRequestBus
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override;
        void ApplyPresetData(const EntitySaveDataContainer& saveDataContainer) override;
        ////

    protected:

        // CommentNodeTextSaveDataInterface
        void OnCommentChanged() override;
        void OnBackgroundColorChanged() override;
        void UpdateStyleOverrides() override;
        ////

    private:
        CommentNodeTextComponent(const CommentNodeTextComponent&) = delete;

        CommentMode                 m_commentMode;
        CommentNodeTextSaveData     m_saveData;

        CommentTextGraphicsWidget*  m_commentTextWidget;
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Qt::AlignmentFlag, "{8CCC83B0-F267-49FE-A9B7-8065F5869E91}")
    AZ_TYPE_INFO_SPECIALIZE(QFont::Style, "{49E7569D-19FE-4BC2-8242-D5DCF5454137}");
    AZ_TYPE_INFO_SPECIALIZE(QFont::Capitalization, "{37EDD868-C58E-4C21-840A-3CE4714CEEA3}");
}
