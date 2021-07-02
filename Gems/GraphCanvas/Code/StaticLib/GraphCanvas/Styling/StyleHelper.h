/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QColor>
#include <QFont>
#include <QFontInfo>
#include <QPen>
#include <QString>
#include <QVariant>
#include <QWidget>
AZ_POP_DISABLE_WARNING

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/std/string/string_view.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Styling/Style.h>
#include <GraphCanvas/Styling/PseudoElement.h>
#include <GraphCanvas/Types/Types.h>
#include <GraphCanvas/Types/QtMetaTypes.h>

namespace GraphCanvas
{
    namespace Styling
    {
        //! Convenience wrapper for a styled entity that resolves its style and then provides easy ways to get common
        //! Qt values out of the style for it.
        class StyleHelper
            : public StyleManagerNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(StyleHelper, AZ::SystemAllocator, 0);

            StyleHelper() = default;

            StyleHelper(const AZ::EntityId& styledEntity)
            {
                SetStyle(styledEntity);
            }

            StyleHelper(const AZ::EntityId& realStyledEntity, const AZStd::string& virtualChildElement)
            {
                SetStyle(realStyledEntity, virtualChildElement);
            }

            virtual ~StyleHelper()
            {
                ReleaseStyle();
            }

            // StyleManagerNotificationBus
            void OnStylesUnloaded() override
            {
                ReleaseStyle();
            }
            ////

            void SetEditorId(const EditorId& editorId)
            {
                if (m_editorId != editorId)
                {
                    ReleaseStyle();
                    m_editorId = editorId;

                    RegisterStyleSheetBus(m_editorId);
                }
            }

            // TODO: Get rid of this and m_scene once the OnSceneSet notification is removed (see below).
            void SetScene(const AZ::EntityId& sceneId)
            {
                m_scene = sceneId;

                EditorId editorId;
                SceneRequestBus::EventResult(editorId, m_scene, &SceneRequests::GetEditorId);
                SetEditorId(editorId);
            }

            void SetStyle(const AZ::EntityId& styledEntity)
            {
                ReleaseStyle();

                m_styledEntity = styledEntity;

                AZ::EntityId sceneId;
                SceneMemberRequestBus::EventResult(sceneId, m_styledEntity, &SceneMemberRequests::GetScene);
                if (!sceneId.IsValid())
                {
                    return;
                }

                SetScene(sceneId);

                for (const auto& selector : m_styleSelectors)
                {
                    StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::AddSelectorState, selector.c_str());
                }

                UpdateStyle();
                
#if 0
                AZStd::string description;
                StyleRequestBus::EventResult(description, m_style, &StyleRequests::GetDescription);
                qDebug() << description.c_str();
#endif
            }

            void SetStyle(const AZStd::string& style)
            {
                ReleaseStyle();

                m_deleteStyledEntity = true;

                PseudoElementFactoryRequestBus::BroadcastResult(m_styledEntity, &PseudoElementFactoryRequests::CreateStyleEntity, style);

                for (const auto& selector : m_styleSelectors)
                {
                    StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::AddSelectorState, selector.c_str());
                }

                // TODO: remove/replace OnSceneSet and fix any systems/components listening for that event.
                SceneMemberNotificationBus::Event(m_styledEntity, &SceneMemberNotifications::OnSceneSet, m_scene);

                UpdateStyle();
            }

            void SetStyle(const AZ::EntityId& parentStyledEntity, const AZStd::string& virtualChildElement)
            {
                ReleaseStyle();

                m_deleteStyledEntity = true;
                
                AZ::EntityId sceneId;
                SceneMemberRequestBus::EventResult(sceneId, parentStyledEntity, &SceneMemberRequests::GetScene);

                SetScene(sceneId);

                PseudoElementFactoryRequestBus::BroadcastResult(m_styledEntity, &PseudoElementFactoryRequests::CreateVirtualChild, parentStyledEntity, virtualChildElement);

                for (const auto& selector : m_styleSelectors)
                {
                    StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::AddSelectorState, selector.c_str());
                }

                UpdateStyle();

#if 0
                AZStd::string description;
                StyleRequestBus::EventResult(description, m_style, &StyleRequests::GetDescription);
                qDebug() << description.c_str();
#endif
            }

            template<typename Value>
            void AddAttributeOverride(Styling::Attribute attribute, const Value& defaultValue = Value())
            {
                m_attributeOverride[attribute] = QVariant(defaultValue);
            }

            void RemoveAttributeOverride(Styling::Attribute attribute)
            {
                m_attributeOverride.erase(attribute);
            }

            bool HasAttribute(Styling::Attribute attribute) const
            {
                bool hasAttribute = (m_attributeOverride.find(attribute) != m_attributeOverride.end());

                if (!hasAttribute)
                {
                    StyleRequestBus::EventResult(hasAttribute, m_style, &StyleRequests::HasAttribute, static_cast<AZ::u32>(attribute));
                }

                return hasAttribute;
            }

            template<typename Value>
            Value GetAttribute(Styling::Attribute attribute, const Value& defaultValue = Value()) const
            {
                Value retVal = defaultValue;

                auto overrideIter = m_attributeOverride.find(attribute);

                if (overrideIter != m_attributeOverride.end())
                {
                    retVal = overrideIter->second.value<Value>();
                }
                else
                {
                    bool hasAttribute = false;
                    auto rawAttribute = static_cast<AZ::u32>(attribute);
                    
                    StyleRequestBus::EventResult(hasAttribute, m_style, &StyleRequests::HasAttribute, rawAttribute);

                    if (hasAttribute)
                    {
                        QVariant variant;
                        StyleRequestBus::EventResult(variant, m_style, &StyleRequests::GetAttribute, rawAttribute);

                        retVal = variant.value<Value>();
                    }
                }

                return retVal;
            }

            QColor GetColor(Styling::Attribute color, QColor defaultValue = QColor()) const
            {
                return GetAttribute(color, defaultValue);
            }

            QFont GetFont() const
            {
                QFont font;
                QFontInfo info(font);
                info.pixelSize();

                font.setFamily(GetAttribute(Attribute::FontFamily, font.family()));
                font.setPixelSize(GetAttribute(Attribute::FontSize, info.pixelSize()));
                font.setWeight(GetAttribute(Attribute::FontWeight, font.weight()));
                font.setStyle(GetAttribute(Attribute::FontStyle, font.style()));
                font.setCapitalization(GetAttribute(Attribute::FontVariant, font.capitalization()));

                return font;
            }

            //! Helper method which constructs a stylesheet based on the calculated font style.
            //! We need this too pass along to certain Qt widgets because we use our own custom style parsing system.
            QString GetFontStyleSheet() const
            {
                QFont font = GetFont();
                QColor color = GetColor(Styling::Attribute::Color);

                QStringList fields;

                fields.push_back(QString("color: rgba(%1,%2,%3,%4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha()));

                fields.push_back(QString("font-family: %1").arg(font.family()));
                fields.push_back(QString("font-size: %1px").arg(font.pixelSize()));

                if (font.bold())
                {
                    fields.push_back("font-weight: bold");
                }

                switch (font.style())
                {
                case QFont::StyleNormal:
                    break;
                case QFont::StyleItalic:
                    fields.push_back("font-style: italic");
                    break;
                case QFont::StyleOblique:
                    fields.push_back("font-style: italic");
                    break;
                }

                const bool underline = font.underline();
                const bool strikeOut = font.strikeOut();

                if (underline && strikeOut)
                {
                    fields.push_back("text-decoration: underline line-through");
                }
                else if (underline)
                {
                    fields.push_back("text-decoration: underline");
                }
                else if (strikeOut)
                {
                    fields.push_back("text-decoration: line-through");
                }

                return fields.join("; ");
            }

            QPen GetPen(Styling::Attribute width, Styling::Attribute style, Styling::Attribute color, Styling::Attribute cap, bool cosmetic = false) const
            {
                QPen pen;
                pen.setColor(GetAttribute(color, QColor(Qt::black)));
                pen.setWidth(GetAttribute(width, 1));
                pen.setStyle(GetAttribute(style, Qt::SolidLine));
                pen.setCapStyle(GetAttribute(cap, Qt::SquareCap));
                pen.setCosmetic(cosmetic);

                return pen;
            }

            QPen GetBorder() const
            {
                return GetPen(Styling::Attribute::BorderWidth, Styling::Attribute::BorderStyle, Styling::Attribute::BorderColor, Styling::Attribute::CapStyle);
            }

            QBrush GetBrush(Styling::Attribute color, QBrush defaultValue = QBrush()) const
            {
                return GetAttribute(color, defaultValue);
            }

            QSizeF GetSize(QSizeF defaultSize) const
            {
                return{
                    GetAttribute(Styling::Attribute::Width, defaultSize.width()),
                    GetAttribute(Styling::Attribute::Height, defaultSize.height())
                };
            }

            QSizeF GetMinimumSize(QSizeF defaultSize = QSizeF(0,0)) const
            {
                return QSizeF(GetAttribute(Styling::Attribute::MinWidth, defaultSize.width()), GetAttribute(Styling::Attribute::MinHeight, defaultSize.height()));
            }

            QSizeF GetMaximumSize(QSizeF defaultSize = QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX)) const
            {
                return QSizeF(GetAttribute(Styling::Attribute::MaxWidth, defaultSize.width()), GetAttribute(Styling::Attribute::MaxHeight, defaultSize.height()));
            }

            QMarginsF GetMargins(QMarginsF defaultMargins = QMarginsF()) const
            {
                bool hasMargin = false;
                StyleRequestBus::EventResult(hasMargin, m_style, &StyleRequests::HasAttribute, static_cast<AZ::u32>(Styling::Attribute::Margin));

                if (hasMargin)
                {
                    qreal defaultMargin = GetAttribute(Styling::Attribute::Margin, 0);
                    defaultMargins = QMarginsF(defaultMargin, defaultMargin, defaultMargin, defaultMargin);
                }

                return QMarginsF(
                    GetAttribute(Styling::Attribute::Margin/*TODO Left*/, defaultMargins.left()),
                    GetAttribute(Styling::Attribute::Margin/*TODO Top*/, defaultMargins.top()),
                    GetAttribute(Styling::Attribute::Margin/*TODO Right*/, defaultMargins.right()),
                    GetAttribute(Styling::Attribute::Margin/*TODO Bottom*/, defaultMargins.bottom())
                );
            }

            bool HasTextAlignment() const
            {
                return HasAttribute(Styling::Attribute::TextAlignment) || HasAttribute(Styling::Attribute::TextVerticalAlignment);
            }

            Qt::Alignment GetTextAlignment(Qt::Alignment defaultAlignment) const
            {
                bool horizontalAlignment = HasAttribute(Styling::Attribute::TextAlignment);
                bool verticalAlignment = HasAttribute(Styling::Attribute::TextVerticalAlignment);

                if (horizontalAlignment || verticalAlignment)
                {
                    Qt::Alignment alignment = GetAttribute(Styling::Attribute::TextAlignment, Qt::AlignmentFlag::AlignLeft);
                    alignment = alignment | GetAttribute(Styling::Attribute::TextVerticalAlignment, Qt::AlignmentFlag::AlignTop);

                    return alignment;
                }

                return defaultAlignment;
            }

            void AddSelector(const AZStd::string_view& selector)
            {
                auto insertResult = m_styleSelectors.insert(AZStd::string(selector));

                if (insertResult.second && m_styledEntity.IsValid())
                {
                    StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::AddSelectorState, selector.data());

                    UpdateStyle();
                }
            }

            void RemoveSelector(const AZStd::string_view& selector)
            {
                AZStd::size_t elements = m_styleSelectors.erase(selector);

                if (elements > 0)
                {
                    StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::RemoveSelectorState, selector.data());

                    UpdateStyle();
                }
            }

            CandyStripeConfiguration GetCandyStripeConfiguration() const
            {
                CandyStripeConfiguration config;

                config.m_initialOffset = GetAttribute(Styling::Attribute::StripeOffset, 0);
                config.m_maximumSize = GetAttribute(Styling::Attribute::MaximumStripeSize, 10);

                if (config.m_maximumSize <= 0)
                {
                    config.m_maximumSize = 1;
                }

                config.m_minStripes = GetAttribute(Styling::Attribute::MinimumStripes, 2);

                if (config.m_minStripes <= 0)
                {
                    config.m_minStripes = 1;
                }

                config.m_stripeAngle = GetAttribute(Styling::Attribute::StripeAngle, 60);

                if (config.m_stripeAngle > 90)
                {
                    config.m_stripeAngle = 89;
                }
                else if (config.m_stripeAngle < -90)
                {
                    config.m_stripeAngle = -89;
                }

                if (!HasAttribute(Styling::Attribute::StripeColor))
                {
                    QColor backgroundColor = GetAttribute(Styling::Attribute::BackgroundColor, QColor(0,0,0));

                    config.m_stripeColor = backgroundColor.darker();

                    int totalDifference = 0;

                    totalDifference += backgroundColor.red() - config.m_stripeColor.red();
                    totalDifference += backgroundColor.green() - config.m_stripeColor.green();
                    totalDifference += backgroundColor.blue() - config.m_stripeColor.blue();

                    if (totalDifference < 150)
                    {
                        config.m_stripeColor = backgroundColor.lighter();
                    }
                }
                else
                {
                    config.m_stripeColor = GetAttribute(Styling::Attribute::StripeColor, QColor(0, 0, 0));
                }

                return config;
            }

            PatternedFillGenerator GetPatternedFillGenerator() const
            {
                PatternedFillGenerator generator;
                generator.m_editorId = m_editorId;

                generator.m_id = GetAttribute(Styling::Attribute::PatternTemplate, QString()).toUtf8().data();

                if (HasAttribute(Styling::Attribute::PatternPalettes))
                {
                    AZStd::string paletteStrings = GetAttribute(Styling::Attribute::PatternPalettes, QString()).toUtf8().data();

                    AzFramework::StringFunc::Tokenize(paletteStrings.c_str(), generator.m_palettes, ',');
                }
                else
                {
                    QColor backgroundColor = GetAttribute(Styling::Attribute::BackgroundColor, QColor(0, 0, 0));

                    QColor patternColor = backgroundColor.darker();

                    int totalDifference = 0;

                    totalDifference += backgroundColor.red() - patternColor.red();
                    totalDifference += backgroundColor.green() - patternColor.green();
                    totalDifference += backgroundColor.blue() - patternColor.blue();

                    if (totalDifference < 150)
                    {
                        patternColor = backgroundColor.lighter();
                    }

                    generator.m_colors.push_back(patternColor);
                }

                generator.m_configuration = GetPatternFillConfiguration();

                return generator;
            }

            PatternFillConfiguration GetPatternFillConfiguration() const
            {
                PatternFillConfiguration configuration;

                configuration.m_minimumTileRepetitions = GetAttribute(Styling::Attribute::MinimumRepetitions, 1);
                configuration.m_evenRowOffsetPercent = GetAttribute(Styling::Attribute::EvenOffsetPercent, 0.0f);
                configuration.m_oddRowOffsetPercent = GetAttribute(Styling::Attribute::OddOffsetPercent, 0.0f);

                return configuration;
            }

            void PopulatePaletteConfiguration(PaletteIconConfiguration& configuration) const
            {
                AZStd::string stylePalette;
                StyledEntityRequestBus::EventResult(stylePalette, m_styledEntity, &StyledEntityRequests::GetFullStyleElement);

                if (!stylePalette.empty())
                {
                    configuration.SetColorPalette(stylePalette);
                }
            }

        private:

            void UpdateStyle()
            {
                ReleaseStyle(false);
                StyleManagerRequestBus::EventResult(m_style, m_editorId, &StyleManagerRequests::ResolveStyles, m_styledEntity);
            }

            void ReleaseStyle(bool destroyChildElement = true)
            {
                if (m_style.IsValid())
                {
                    if (m_deleteStyledEntity && destroyChildElement)
                    {
                        m_deleteStyledEntity = false;
                        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, m_styledEntity);
                    }
                    AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, m_style);

                    m_style.SetInvalid();
                }
            }

            void RegisterStyleSheetBus(const EditorId& editorId)
            {
                StyleManagerNotificationBus::Handler::BusDisconnect();
                StyleManagerNotificationBus::Handler::BusConnect(editorId);
            }
            
            EditorId m_editorId;
            AZ::EntityId m_scene;
            AZ::EntityId m_styledEntity;
            AZ::EntityId m_style;

            bool m_deleteStyledEntity = false;

            AZStd::unordered_set< AZStd::string > m_styleSelectors;
            AZStd::unordered_map< Attribute, QVariant > m_attributeOverride;
        };

    }
}
