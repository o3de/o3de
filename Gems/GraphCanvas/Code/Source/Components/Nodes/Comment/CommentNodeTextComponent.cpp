/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QGraphicsItem>
#include <QGraphicsGridLayout>
#include <QGraphicsLayoutItem>
#include <QGraphicsScene>
#include <QGraphicsWidget>
#include <QPainter>
AZ_POP_DISABLE_WARNING

#include <AzCore/RTTI/TypeInfo.h>

#include <Components/Nodes/Comment/CommentNodeTextComponent.h>

#include <Components/Nodes/Comment/CommentTextGraphicsWidget.h>
#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <GraphCanvas/Components/GridBus.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

namespace GraphCanvas
{
    /////////////////////////////
    // CommentNodeTextComponent
    /////////////////////////////
    bool CommentNodeTextComponentVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 2)
        {
            AZ::Crc32 commentId = AZ_CRC_CE("Comment");
            AZ::Crc32 fontId = AZ_CRC_CE("FontSettings");

            CommentNodeTextSaveData saveData;

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(commentId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_comment);
            }

            dataNode = nullptr;
            dataNode = classElement.FindSubElement(fontId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_fontConfiguration);
            }

            classElement.RemoveElementByName(commentId);
            classElement.RemoveElementByName(fontId);

            classElement.AddElementWithData(context, "SaveData", saveData);
        }

        return true;
    }

    void CommentNodeTextComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CommentNodeTextSaveData, ComponentSaveData>()
                ->Version(2)
                ->Field("Comment", &CommentNodeTextSaveData::m_comment)
                ->Field("BackgroundColor", &CommentNodeTextSaveData::m_backgroundColor)
                ->Field("FontSettings", &CommentNodeTextSaveData::m_fontConfiguration)
                ;

            serializeContext->Class<CommentNodeTextComponent, GraphCanvasPropertyComponent>()
                ->Version(3, &CommentNodeTextComponentVersionConverter)
                ->Field("SaveData", &CommentNodeTextComponent::m_saveData)
            ;

            serializeContext->Class<FontConfiguration>()
                ->Field("FontColor", &FontConfiguration::m_fontColor)
                ->Field("FontFamily", &FontConfiguration::m_fontFamily)
                ->Field("PixelSize", &FontConfiguration::m_pixelSize)
                ->Field("Weight", &FontConfiguration::m_weight)
                ->Field("Style", &FontConfiguration::m_style)
                ->Field("VAlign", &FontConfiguration::m_verticalAlignment)
                ->Field("HAlign", &FontConfiguration::m_horizontalAlignment)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class < CommentNodeTextSaveData >("SaveData", "The save information regarding a comment node")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CommentNodeTextSaveData::m_comment, "Title", "The comment to display on this node")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CommentNodeTextSaveData::OnCommentChanged)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CommentNodeTextSaveData::GetCommentLabel)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CommentNodeTextSaveData::m_backgroundColor, "Background Color", "The background color to display the node comment on")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CommentNodeTextSaveData::OnBackgroundColorChanged)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CommentNodeTextSaveData::GetBackgroundLabel)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CommentNodeTextSaveData::m_fontConfiguration, "Font Settings", "The font settings used to render the font in the comment.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CommentNodeTextSaveData::UpdateStyleOverrides)
                ;

                editContext->Class<CommentNodeTextComponent>("Comment", "The node's customizable properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CommentNodeTextComponent::m_saveData, "SaveData", "The modifiable information about this comment.")
                ;

                editContext->Class<FontConfiguration>("Font Settings", "Various settings used to control a font.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &FontConfiguration::m_fontColor, "Font Color", "The color that the font of this comment should render with")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &FontConfiguration::m_fontFamily, "Font Family", "The font family to use when rendering this comment.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &FontConfiguration::m_pixelSize, "Pixel Size", "The size of the font(in pixels)")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 200)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FontConfiguration::m_weight, "Weight", "The weight of the font")
                        ->EnumAttribute(QFont::Thin, "Thin")
                        ->EnumAttribute(QFont::ExtraLight, "Extra Light")
                        ->EnumAttribute(QFont::Light, "Light")
                        ->EnumAttribute(QFont::Normal, "Normal")
                        ->EnumAttribute(QFont::Medium, "Medium")
                        ->EnumAttribute(QFont::DemiBold, "Demi-Bold")
                        ->EnumAttribute(QFont::Bold, "Bold")
                        ->EnumAttribute(QFont::ExtraBold, "Extra Bold")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FontConfiguration::m_style, "Style", "The style of the font")
                        ->EnumAttribute(QFont::Style::StyleNormal, "Normal")
                        ->EnumAttribute(QFont::Style::StyleItalic, "Italic")
                        ->EnumAttribute(QFont::Style::StyleOblique, "Oblique")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FontConfiguration::m_verticalAlignment, "Vertical Alignment", "The Vertical Alignment of the font")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignTop, "Top")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignVCenter, "Middle")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignBottom, "Bottom")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FontConfiguration::m_horizontalAlignment, "Horizontal Alignment", "The Horizontal Alignment of the font")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignLeft, "Left")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignHCenter, "Center")
                        ->EnumAttribute(Qt::AlignmentFlag::AlignRight, "Right")
                    ;
            }
        }
    }

    CommentNodeTextComponent::CommentNodeTextComponent()
        : m_saveData(this)
        , m_commentTextWidget(nullptr)
        , m_commentMode(CommentMode::Comment)        
    {
    }

    CommentNodeTextComponent::CommentNodeTextComponent(AZStd::string_view initialText)
        : CommentNodeTextComponent()
    {
        m_saveData.m_comment = initialText;
    }

    void CommentNodeTextComponent::Init()
    {
        GraphCanvasPropertyComponent::Init();
        
        m_saveData.m_fontConfiguration.InitializePixelSize();
        EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
    }

    void CommentNodeTextComponent::Activate()
    {
        if (m_commentTextWidget == nullptr)
        {
            m_commentTextWidget = aznew CommentTextGraphicsWidget(GetEntityId());
            m_commentTextWidget->SetStyle(Styling::Elements::CommentText);
        }

        GraphCanvasPropertyComponent::Activate();

        CommentRequestBus::Handler::BusConnect(GetEntityId());
        CommentLayoutRequestBus::Handler::BusConnect(GetEntityId());

        NodeNotificationBus::Handler::BusConnect(GetEntityId());
        
        m_commentTextWidget->Activate();
    }

    void CommentNodeTextComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        NodeNotificationBus::Handler::BusDisconnect();

        CommentLayoutRequestBus::Handler::BusDisconnect();
        CommentRequestBus::Handler::BusDisconnect();

        m_commentTextWidget->Deactivate();
    }

    void CommentNodeTextComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetSnapToGrid, true);

        if (m_commentTextWidget->GetCommentMode() == CommentMode::Comment)
        {
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetResizeToGrid, false);
        }
        else if (m_commentTextWidget->GetCommentMode() == CommentMode::BlockComment)
        {
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetResizeToGrid, true);
        }

        AZ::EntityId grid;
        SceneRequestBus::EventResult(grid, sceneId, &SceneRequests::GetGrid);

        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetGrid, grid);        

        m_commentTextWidget->OnAddedToScene();
        UpdateStyleOverrides();
        OnCommentChanged();
        OnBackgroundColorChanged();

        m_saveData.RegisterIds(GetEntityId(), sceneId);
    }

    void CommentNodeTextComponent::SetComment(const AZStd::string& comment)
    {
        if (m_saveData.m_comment.compare(comment) != 0)
        {
            m_saveData.m_comment = comment;
            m_commentTextWidget->SetComment(comment);

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            GraphModelRequestBus::Event(sceneId, &GraphModelRequests::RequestUndoPoint);
        }
    }

    void CommentNodeTextComponent::SetCommentMode(CommentMode commentMode)
    {
        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetSnapToGrid, true);
        m_commentTextWidget->SetCommentMode(commentMode);
        m_commentMode = commentMode;

        if (m_commentTextWidget->GetCommentMode() == CommentMode::Comment)
        {
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetResizeToGrid, false);
        }
        else if (m_commentTextWidget->GetCommentMode() == CommentMode::BlockComment)
        {
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetResizeToGrid, true);
        }
    }

    void CommentNodeTextComponent::SetBackgroundColor(const AZ::Color& backgroundColor)
    {
        m_saveData.m_backgroundColor = backgroundColor;
        m_saveData.SignalDirty();

        OnBackgroundColorChanged();
    }

    AZ::Color CommentNodeTextComponent::GetBackgroundColor() const
    {
        return m_saveData.m_backgroundColor;
    }

    CommentMode CommentNodeTextComponent::GetCommentMode() const
    {
        return m_commentMode;
    }

    const AZStd::string& CommentNodeTextComponent::GetComment() const
    {
        return m_saveData.m_comment;
    }

    QGraphicsLayoutItem* CommentNodeTextComponent::GetGraphicsLayoutItem()
    {
        return m_commentTextWidget;
    }

    void CommentNodeTextComponent::WriteSaveData(EntitySaveDataContainer& saveDataContainer) const
    {
        CommentNodeTextSaveData* saveData = saveDataContainer.FindCreateSaveData<CommentNodeTextSaveData>();

        if (saveData)
        {
            (*saveData) = m_saveData;
        }
    }

    void CommentNodeTextComponent::ReadSaveData(const EntitySaveDataContainer& saveDataContainer)
    {
        CommentNodeTextSaveData* saveData = saveDataContainer.FindSaveDataAs<CommentNodeTextSaveData>();

        if (saveData)
        {
            m_saveData = (*saveData);
        }
    }

    void CommentNodeTextComponent::ApplyPresetData(const EntitySaveDataContainer& saveDataContainer)
    {
        CommentNodeTextSaveData* saveData = saveDataContainer.FindSaveDataAs<CommentNodeTextSaveData>();

        if (saveData)
        {
            // Copy over everything but the save comment.
            AZStd::string previousComment = m_saveData.m_comment;

            m_saveData = (*saveData);

            m_saveData.m_comment = previousComment;

            UpdateStyleOverrides();
            OnBackgroundColorChanged();
        }
    }

    void CommentNodeTextComponent::OnCommentChanged()
    {
        if (m_commentTextWidget)
        {
            m_commentTextWidget->SetComment(m_saveData.m_comment);
            CommentNotificationBus::Event(GetEntityId(), &CommentNotifications::OnCommentChanged, m_saveData.m_comment);
        }
    }

    void CommentNodeTextComponent::OnBackgroundColorChanged()
    {
        CommentNotificationBus::Event(GetEntityId(), &CommentNotifications::OnBackgroundColorChanged, m_saveData.m_backgroundColor);
    }

    void CommentNodeTextComponent::UpdateStyleOverrides()
    {
        CommentNotificationBus::Event(GetEntityId(), &CommentNotifications::OnCommentFontReloadBegin);

        QColor fontColor = ConversionUtils::AZToQColor(m_saveData.m_fontConfiguration.m_fontColor);

        Styling::StyleHelper& styleHelper = m_commentTextWidget->GetStyleHelper();

        styleHelper.AddAttributeOverride(Styling::Attribute::Color, fontColor);
        styleHelper.AddAttributeOverride(Styling::Attribute::FontFamily, QString(m_saveData.m_fontConfiguration.m_fontFamily.c_str()));
        styleHelper.AddAttributeOverride(Styling::Attribute::FontSize, m_saveData.m_fontConfiguration.m_pixelSize);
        styleHelper.AddAttributeOverride(Styling::Attribute::FontWeight, m_saveData.m_fontConfiguration.m_weight);
        styleHelper.AddAttributeOverride(Styling::Attribute::FontStyle, m_saveData.m_fontConfiguration.m_style);
        styleHelper.AddAttributeOverride(Styling::Attribute::TextAlignment, m_saveData.m_fontConfiguration.m_horizontalAlignment);
        styleHelper.AddAttributeOverride(Styling::Attribute::TextVerticalAlignment, m_saveData.m_fontConfiguration.m_verticalAlignment);

        m_commentTextWidget->OnStyleChanged();

        CommentNotificationBus::Event(GetEntityId(), &CommentNotifications::OnCommentFontReloadEnd);
    }
}
