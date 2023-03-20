/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)

#include <QColor>
#include <QFont>
#include <QFontInfo>
#include <QPen>
#include <QString>
#include <QVariant>
#include <QWidget>

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

#endif

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
            AZ_CLASS_ALLOCATOR(StyleHelper, AZ::SystemAllocator);

            StyleHelper() = default;
            StyleHelper(const AZ::EntityId& styledEntity);
            StyleHelper(const AZ::EntityId& realStyledEntity, const AZStd::string& virtualChildElement);
            virtual ~StyleHelper();

            // StyleManagerNotificationBus
            void OnStylesUnloaded() override;
            ////

            void SetEditorId(const EditorId& editorId);
            void SetScene(const AZ::EntityId& sceneId);

            void SetStyle(const AZStd::string& style);
            void SetStyle(const AZ::EntityId& styledEntity);
            void SetStyle(const AZ::EntityId& parentStyledEntity, const AZStd::string& virtualChildElement);

            bool HasAttribute(Styling::Attribute attribute) const;
            void RemoveAttributeOverride(Styling::Attribute attribute);

            QColor GetColor(Styling::Attribute color, QColor defaultValue = QColor()) const;
            QFont GetFont() const;

            //! Helper method which constructs a stylesheet based on the calculated font style.
            //! We need this too pass along to certain Qt widgets because we use our own custom style parsing system.
            QString GetFontStyleSheet() const;

            QPen GetPen(Styling::Attribute width, Styling::Attribute style, Styling::Attribute color, Styling::Attribute cap, bool cosmetic = false) const;
            QPen GetBorder() const;
            QBrush GetBrush(Styling::Attribute color, QBrush defaultValue = QBrush()) const;
            QSizeF GetSize(QSizeF defaultSize) const;
            QSizeF GetMinimumSize(QSizeF defaultSize = QSizeF(0,0)) const;
            QSizeF GetMaximumSize(QSizeF defaultSize = QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX)) const;
            QMarginsF GetMargins(QMarginsF defaultMargins = QMarginsF()) const;

            bool HasTextAlignment() const;
            Qt::Alignment GetTextAlignment(Qt::Alignment defaultAlignment) const;

            void AddSelector(const AZStd::string_view& selector);
            void RemoveSelector(const AZStd::string_view& selector);

            CandyStripeConfiguration GetCandyStripeConfiguration() const;
            PatternedFillGenerator GetPatternedFillGenerator() const;
            PatternFillConfiguration GetPatternFillConfiguration() const;
            void PopulatePaletteConfiguration(PaletteIconConfiguration& configuration) const;

            template<typename Value>
            void AddAttributeOverride(Styling::Attribute attribute, const Value& defaultValue = Value())
            {
                m_attributeOverride[attribute] = QVariant(defaultValue);
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

        private:

            void UpdateStyle();
            void ReleaseStyle(bool destroyChildElement = true);
            void RegisterStyleSheetBus(const EditorId& editorId);
            
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
