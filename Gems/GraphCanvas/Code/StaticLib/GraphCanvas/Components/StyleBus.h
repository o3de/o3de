/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Styling/Selector.h>

class QColor;
class QPixmap;
class QVariant;

namespace GraphCanvas
{
    namespace Styling
    {
        class StyleHelper;
    }

    static const AZ::Crc32 StyledGraphicItemServiceCrc = AZ_CRC_CE("GraphCanvas_StyledGraphicItemService");

    ///////////////////////////////////////////////////////////////////////////////////
    //! StyledEntityRequests
    //! Provide details about an entity to support it being styled.
    class StyledEntityRequests : public AZ::EBusTraits
    {
    public:
        // Allow any number of handlers per address.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;


        //! If this entity has a parent that is also styled, get its ID, otherwise AZ::EntityId()
        virtual AZ::EntityId GetStyleParent() const = 0;

        //! Get a set of styling selectors applicable for the entity
        virtual Styling::SelectorVector GetStyleSelectors() const = 0;

        virtual void AddSelectorState(const char* selector) = 0;
        virtual void RemoveSelectorState(const char* selector) = 0;

        //! Get the "style element" that the entity "is"; e.g. "node", "slot", "connection", etc.
        virtual AZStd::string GetElement() const = 0;
        //! Get the "style class" that the entity has. This should start with a '.' and contain [A-Za-z_-].
        virtual AZStd::string GetClass() const = 0;

        //! Returns <element>.<class>
        virtual AZStd::string GetFullStyleElement() const;
    };

    using StyledEntityRequestBus = AZ::EBus<StyledEntityRequests>;
    ///////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////
    // StyleManager
    // Requests
    class PaletteIconConfiguration
    {
    public:
        AZStd::string m_iconPalette;

        float m_transitionPercent = 0.1f;

        void ClearPalettes();
        void ReservePalettes(size_t size);
        void SetColorPalette(AZStd::string_view paletteString);
        void AddColorPalette(AZStd::string_view paletteString);
        const AZStd::vector< AZStd::string >& GetColorPalettes() const;
        AZ::Crc32 GetPaletteCrc() const;

    private:
        AZ::Crc32 m_paletteCrc;
        AZStd::vector< AZStd::string > m_colorPalettes;
    };

    class StyleManagerRequests : public AZ::EBusTraits
    {    
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorId;

        //! Match the selectors an entity has against known styles and provide an aggregate meta - style for it.
        virtual AZ::EntityId ResolveStyles(const AZ::EntityId& object) const = 0;

        virtual void RegisterDataPaletteStyle(const AZ::Uuid& dataType, const AZStd::string& palette) = 0;

        virtual AZStd::string GetDataPaletteStyle(const AZ::Uuid& dataType) const = 0;
        virtual const Styling::StyleHelper* FindDataColorPalette(const AZ::Uuid& uuid) = 0;
        virtual QColor GetDataTypeColor(const AZ::Uuid& dataType) = 0;
        virtual const QPixmap* GetDataTypeIcon(const AZ::Uuid& dataType) = 0;
        virtual const QPixmap* GetMultiDataTypeIcon(const AZStd::vector<AZ::Uuid>& dataTypes) = 0;

        virtual const Styling::StyleHelper* FindColorPalette(const AZStd::string& paletteString) = 0;
        virtual QColor GetPaletteColor(const AZStd::string& palette) = 0;
        virtual const QPixmap* GetPaletteIcon(const AZStd::string& iconStyle, const AZStd::string& palette) = 0;
        virtual const QPixmap* GetConfiguredPaletteIcon(const PaletteIconConfiguration& paletteConfiguration) = 0;
        virtual const Styling::StyleHelper* FindPaletteIconStyleHelper(const PaletteIconConfiguration& paletteConfiguration) = 0;

        virtual QPixmap* CreateIcon(const QColor& colorType, const AZStd::string& iconStyle) = 0;
        virtual QPixmap* CreateIconFromConfiguration(const PaletteIconConfiguration& paletteConfiguration) = 0;
        virtual QPixmap* CreateMultiColoredIcon(const AZStd::vector<QColor>& colorType, float transitionPercent, const AZStd::string& iconStyle) = 0;

        virtual QPixmap* CreateColoredPatternPixmap(const AZStd::vector< QColor >& colorTypes, const AZStd::string& patternKey) = 0;
        virtual const QPixmap* CreatePatternPixmap(const AZStd::vector< AZStd::string >& palettes, const AZStd::string& patternKey) = 0;

        virtual AZStd::vector<AZStd::string> GetColorPaletteStyles() const = 0;

        // Pixmap Caching Mechanism
        virtual QPixmap* FindPixmap(const AZ::Crc32& keyName) = 0;
        virtual void CachePixmap(const AZ::Crc32& keyName, QPixmap* pixmap) = 0;

        // Layering
        virtual int FindLayer(AZStd::string_view layer) = 0;

        // Size Stepping
        virtual int GetSteppedWidth(int gridSteps) = 0;
        virtual int GetSteppedHeight(int gridSteps) = 0;
    };

    using StyleManagerRequestBus = AZ::EBus<StyleManagerRequests>;

    //! Notifications
    class StyleManagerNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorId;

        virtual void OnStylesUnloaded() {}
        virtual void OnStylesLoaded() {}
    };

    using StyleManagerNotificationBus = AZ::EBus<StyleManagerNotifications>;
    ///////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////
    //! StyleRequests
    //! Get the style for an entity (per its current state)
    class StyleRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Get a textual description of the style, useful for debugging.
        virtual AZStd::string GetDescription() const = 0;

        //! Check whether the style has a given attribute.
        virtual bool HasAttribute(AZ::u32 attribute) const = 0;
        //! Get an attribute from a style. If the style lacks the attribute, QVariant() will be returned.
        virtual QVariant GetAttribute(AZ::u32 attribute) const = 0;
    };

    using StyleRequestBus = AZ::EBus<StyleRequests>;

    //! StyleNtofications
    //! Notifications about changes to the style
    class StyleNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! The style changed.
        virtual void OnStyleChanged() = 0;
    };

    using StyleNotificationBus = AZ::EBus<StyleNotifications>;
    ///////////////////////////////////////////////////////////////////////////////////

}

DECLARE_EBUS_EXTERN(GraphCanvas::StyleManagerNotifications);
