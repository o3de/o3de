/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

#include <GraphCanvas/Components/StyleBus.h>

#include <GraphCanvas/Types/SceneMemberComponentSaveData.h>

class QGraphicsWidget;

namespace GraphCanvas
{
    static const AZ::Crc32 NodeTitleServiceCrc = AZ_CRC_CE("GraphCanvas_TitleService");

    //! NodeTitleRequests
    //! Requests that get/set an entity's Node Title
    //
    // Most of these pushes should become pulls to avoid needing to over expose information in this bus.
    // May also come up with a way changing up the tag type here so we can pull specific widgets from a generic bus
    // to improve the customization.
    class NodeTitleRequests
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual QGraphicsWidget* GetGraphicsWidget() = 0;

        //! Set the node's details, title, subtitle, tooltip
        virtual void SetDetails(const AZStd::string& title, const AZStd::string& subtitle) = 0;

        //! Set the Node's title.
        virtual void SetTitle(const AZStd::string& value) = 0;

        //! Get the Node's title.
        virtual AZStd::string GetTitle() const = 0;

        //! Set the Node's sub-title.
        virtual void SetSubTitle(const AZStd::string& value) = 0;

        //! Get the Node's sub-title.
        virtual AZStd::string GetSubTitle() const = 0;

        //! Sets the base palette for the title. This won't be saved out.
        virtual void SetDefaultPalette(const AZStd::string& basePalette) = 0;

        //! Sets an override for the palette. This will be saved out.
        virtual void SetPaletteOverride(const AZStd::string& paletteOverride) = 0;
        virtual void SetDataPaletteOverride(const AZ::Uuid& uuid) = 0;
        virtual void SetColorPaletteOverride(const QColor& color) = 0;

        virtual void ConfigureIconConfiguration(PaletteIconConfiguration& paletteConfiguration) = 0;

        virtual void ClearPaletteOverride() = 0;
    };

    using NodeTitleRequestBus = AZ::EBus<NodeTitleRequests>;

    //! NodeTitleNotifications
    //! Notifications about changes to the state of a Node Title
    class NodeTitleNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnTitleChanged() = 0;
    };

    using NodeTitleNotificationsBus = AZ::EBus<NodeTitleNotifications>;

    class GeneralNodeTitleComponentSaveData;
    AZ_TYPE_INFO_SPECIALIZE(GeneralNodeTitleComponentSaveData, "{328FF15C-C302-458F-A43D-E1794DE0904E}");

    class GeneralNodeTitleComponentSaveData
        : public SceneMemberComponentSaveData<GeneralNodeTitleComponentSaveData>
    {
    public:
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR(GeneralNodeTitleComponentSaveData, AZ::SystemAllocator);

        GeneralNodeTitleComponentSaveData() = default;
        ~GeneralNodeTitleComponentSaveData() = default;

        bool RequiresSave() const override
        {
            return !m_paletteOverride.empty();
        }

        AZStd::string m_paletteOverride;
    };
}
