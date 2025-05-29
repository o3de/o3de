/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/DeviceDrawPacket.h>
#include <Atom/RHI/GeometryView.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

// Predefinition for unit test friend class
namespace UnitTest
{
    class MultiDeviceDrawPacketTest;
}

namespace AZ::RHI
{
    //! DrawPacket is a multi-device class that holds a map of device-specific DrawPackets as well as
    //! a vector of MultiDeviceDrawItems, corresponding SortKeys, DrawListTags and DrawListMasks.
    //! A DrawPacket is only intened to be contructed via the DrawPacketBuilder.
    //! Individual device-specific DrawPackets are allocated as packed data structures, referenced via RHI::Ptrs
    //! in a map, indexed by the device-index.
    class DrawPacket final : public AZStd::intrusive_base
    {
        friend class DrawPacketBuilder;
        friend class UnitTest::MultiDeviceDrawPacketTest;

    public:
        using DrawItemVisitor = AZStd::function<void(DrawListTag, DrawItemProperties)>;

        //! Draw packets cannot be move constructed or copied, as they contain an additional memory payload.
        //! Use DeviceDrawPacketBuilder::Clone to copy a draw packet.
        AZ_DISABLE_COPY_MOVE(DrawPacket);

        //! Returns the mask representing all the draw lists affected by the packet.
        DrawListMask GetDrawListMask() const;

        //! Returns the number of draw items stored in the packet.
        size_t GetDrawItemCount() const;

        //! Returns the index associated with the given DrawListTag
        s32 GetDrawListIndex(DrawListTag drawListTag) const;

        //! Returns the DeviceDrawItem at the given index
        DrawItem* GetDrawItem(size_t index);
        const DrawItem* GetDrawItem(size_t index) const;

        //! Returns the DeviceDrawItem associated with the given DrawListTag
        DrawItem* GetDrawItem(DrawListTag drawListTag);
        const DrawItem* GetDrawItem(DrawListTag drawListTag) const;

        //! Returns the draw item and its properties associated with the provided index.
        DrawItemProperties GetDrawItemProperties(size_t index) const;

        //! Returns the draw list tag associated with the provided index, used to filter the draw item into an appropriate pass.
        DrawListTag GetDrawListTag(size_t index) const;

        //! Returns the draw filter mask associated with the provided index, used to filter the draw item into an appropriate render
        //! pipeline.
        DrawFilterMask GetDrawFilterMask(size_t index) const;

        //! Update the root constant at the specified interval. The same root constants are shared by all draw items in the draw packet
        void SetRootConstant(uint32_t offset, const AZStd::span<u8>& data);

        //! Set the instance count in all draw items.
        void SetInstanceCount(uint32_t instanceCount);

        const DeviceDrawPacket* GetDeviceDrawPacket(int deviceIndex) const
        {
            AZ_Error(
                "DrawPacket",
                m_deviceDrawPackets.find(deviceIndex) != m_deviceDrawPackets.end(),
                "No DeviceDrawPacket found for device index %d\n",
                deviceIndex);

            return m_deviceDrawPackets.at(deviceIndex).get();
        }

    private:
        //! Use DeviceDrawPacketBuilder to construct an instance.
        DrawPacket() = default;

        //! The bit-mask of all active filter tags.
        DrawListMask m_drawListMask{};

        //! List of draw items.
        AZStd::vector<DrawItem> m_drawItems;

        //! List of draw item sort keys associated with the draw item index.
        AZStd::vector<DrawItemSortKey> m_drawItemSortKeys;

        //! List of draw list tags associated with the draw item index.
        AZStd::vector<DrawListTag> m_drawListTags;

        //! List of draw filter masks associated with the draw item index.
        AZStd::vector<DrawFilterMask> m_drawFilterMasks;

        //! A map of single-device DrawPackets, index by the device index
        AZStd::unordered_map<int, RHI::Ptr<DeviceDrawPacket>> m_deviceDrawPackets;
    };
} // namespace AZ::RHI
