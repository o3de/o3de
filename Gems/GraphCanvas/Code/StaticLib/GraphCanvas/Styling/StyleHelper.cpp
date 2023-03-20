/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Styling/StyleHelper.h>

#include <QDebug>

namespace GraphCanvas
{
    namespace Styling
    {

        StyleHelper::StyleHelper(const AZ::EntityId& styledEntity)
        {
            SetStyle(styledEntity);
        }

        StyleHelper::StyleHelper(const AZ::EntityId& realStyledEntity, const AZStd::string& virtualChildElement)
        {
            SetStyle(realStyledEntity, virtualChildElement);
        }

        StyleHelper::~StyleHelper()
        {
            ReleaseStyle();
        }

        void StyleHelper::OnStylesUnloaded()
        {
            ReleaseStyle();
        }

        void StyleHelper::SetEditorId(const EditorId& editorId)
        {
            if (m_editorId != editorId)
            {
                ReleaseStyle();
                m_editorId = editorId;

                RegisterStyleSheetBus(m_editorId);
            }
        }

        void StyleHelper::SetScene(const AZ::EntityId& sceneId)
        {
            m_scene = sceneId;

            EditorId editorId;
            SceneRequestBus::EventResult(editorId, m_scene, &SceneRequests::GetEditorId);
            SetEditorId(editorId);
        }

        void StyleHelper::SetStyle(const AZ::EntityId& styledEntity)
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
        }

        void StyleHelper::SetStyle(const AZStd::string& style)
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

            static bool enableDiagnostic = false;
            if (enableDiagnostic)
            {
                AZStd::string description;
                StyleRequestBus::EventResult(description, m_style, &StyleRequests::GetDescription);
                qDebug() << description.c_str();
            }
        }

        void StyleHelper::SetStyle(const AZ::EntityId& parentStyledEntity, const AZStd::string& virtualChildElement)
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

            static bool enableDiagnostic = false;
            if (enableDiagnostic)
            {
                AZStd::string description;
                StyleRequestBus::EventResult(description, m_style, &StyleRequests::GetDescription);
                qDebug() << description.c_str();
            }

        }

        void StyleHelper::RemoveAttributeOverride(Styling::Attribute attribute)
        {
            m_attributeOverride.erase(attribute);
        }

        bool StyleHelper::HasAttribute(Styling::Attribute attribute) const
        {
            bool hasAttribute = (m_attributeOverride.find(attribute) != m_attributeOverride.end());

            if (!hasAttribute)
            {
                StyleRequestBus::EventResult(hasAttribute, m_style, &StyleRequests::HasAttribute, static_cast<AZ::u32>(attribute));
            }

            return hasAttribute;
        }

        QColor StyleHelper::GetColor(Styling::Attribute color, QColor defaultValue /*= QColor()*/) const
        {
            return GetAttribute(color, defaultValue);
        }

        QFont StyleHelper::GetFont() const
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

        QString StyleHelper::GetFontStyleSheet() const
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

        QPen StyleHelper::GetPen(Styling::Attribute width, Styling::Attribute style, Styling::Attribute color, Styling::Attribute cap, bool cosmetic /*= false*/) const
        {
            QPen pen;
            pen.setColor(GetAttribute(color, QColor(Qt::black)));
            pen.setWidth(GetAttribute(width, 1));
            pen.setStyle(GetAttribute(style, Qt::SolidLine));
            pen.setCapStyle(GetAttribute(cap, Qt::SquareCap));
            pen.setCosmetic(cosmetic);

            return pen;
        }

        QPen StyleHelper::GetBorder() const
        {
            return GetPen(Styling::Attribute::BorderWidth, Styling::Attribute::BorderStyle, Styling::Attribute::BorderColor, Styling::Attribute::CapStyle);
        }

        QBrush StyleHelper::GetBrush(Styling::Attribute color, QBrush defaultValue /*= QBrush()*/) const
        {
            return GetAttribute(color, defaultValue);
        }

        QSizeF StyleHelper::GetSize(QSizeF defaultSize) const
        {
            return{
                GetAttribute(Styling::Attribute::Width, defaultSize.width()),
                GetAttribute(Styling::Attribute::Height, defaultSize.height())
            };
        }

        QSizeF StyleHelper::GetMinimumSize(QSizeF defaultSize /*= QSizeF(0,0)*/) const
        {
            return QSizeF(GetAttribute(Styling::Attribute::MinWidth, defaultSize.width()), GetAttribute(Styling::Attribute::MinHeight, defaultSize.height()));
        }

        QSizeF StyleHelper::GetMaximumSize(QSizeF defaultSize /*= QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX)*/) const
        {
            return QSizeF(GetAttribute(Styling::Attribute::MaxWidth, defaultSize.width()), GetAttribute(Styling::Attribute::MaxHeight, defaultSize.height()));
        }

        QMarginsF StyleHelper::GetMargins(QMarginsF defaultMargins /*= QMarginsF()*/) const
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

        bool StyleHelper::HasTextAlignment() const
        {
            return HasAttribute(Styling::Attribute::TextAlignment) || HasAttribute(Styling::Attribute::TextVerticalAlignment);
        }

        Qt::Alignment StyleHelper::GetTextAlignment(Qt::Alignment defaultAlignment) const
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

        void StyleHelper::AddSelector(const AZStd::string_view& selector)
        {
            auto insertResult = m_styleSelectors.insert(AZStd::string(selector));

            if (insertResult.second && m_styledEntity.IsValid())
            {
                StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::AddSelectorState, selector.data());

                UpdateStyle();
            }
        }

        void StyleHelper::RemoveSelector(const AZStd::string_view& selector)
        {
            AZStd::size_t elements = m_styleSelectors.erase(selector);

            if (elements > 0)
            {
                StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::RemoveSelectorState, selector.data());

                UpdateStyle();
            }
        }

        GraphCanvas::CandyStripeConfiguration StyleHelper::GetCandyStripeConfiguration() const
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
                QColor backgroundColor = GetAttribute(Styling::Attribute::BackgroundColor, QColor(0, 0, 0));

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

        GraphCanvas::PatternedFillGenerator StyleHelper::GetPatternedFillGenerator() const
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

        GraphCanvas::PatternFillConfiguration StyleHelper::GetPatternFillConfiguration() const
        {
            PatternFillConfiguration configuration;

            configuration.m_minimumTileRepetitions = GetAttribute(Styling::Attribute::MinimumRepetitions, 1);
            configuration.m_evenRowOffsetPercent = GetAttribute(Styling::Attribute::EvenOffsetPercent, 0.0f);
            configuration.m_oddRowOffsetPercent = GetAttribute(Styling::Attribute::OddOffsetPercent, 0.0f);

            return configuration;
        }

        void StyleHelper::PopulatePaletteConfiguration(PaletteIconConfiguration& configuration) const
        {
            AZStd::string stylePalette;
            StyledEntityRequestBus::EventResult(stylePalette, m_styledEntity, &StyledEntityRequests::GetFullStyleElement);

            if (!stylePalette.empty())
            {
                configuration.SetColorPalette(stylePalette);
            }
        }

        void StyleHelper::UpdateStyle()
        {
            ReleaseStyle(false);
            StyleManagerRequestBus::EventResult(m_style, m_editorId, &StyleManagerRequests::ResolveStyles, m_styledEntity);
        }

        void StyleHelper::ReleaseStyle(bool destroyChildElement /*= true*/)
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

        void StyleHelper::RegisterStyleSheetBus(const EditorId& editorId)
        {
            StyleManagerNotificationBus::Handler::BusDisconnect();
            StyleManagerNotificationBus::Handler::BusConnect(editorId);
        }

}
}
