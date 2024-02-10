/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DrawList.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

// Predefinition for unit test friend class
namespace UnitTest
{
    class DrawPacketTest;
    class MultiDeviceDrawPacketTest;
}

namespace AZ
{
    class IAllocator;
}

namespace AZ::RHI
{
    //!
    //! DrawPacket is a packed data structure (one contiguous allocation) containing a collection of
    //! DrawItems and their associated array data. Each draw item in the packet is associated
    //! with a DrawListTag. All draw items in the packet share the same set of shader resource
    //! groups, index buffer, one DrawFilterMask, and draw arguments.
    //! 
    //! Some notes about design and usage:
    //!   - Draw packets should be used to 'broadcast' variations of the same 'object' to multiple passes.
    //!     For example: 'Shadow', 'Depth', 'Forward'.
    //! 
    //!   - Draw packets can be re-used between different views, scenes, or passes. The embedded shader resource groups
    //!     should represent only the local data necessary to describe the 'object', not the full context including
    //!     scene / view / pass specific state. They serve as a 'template'.
    //! 
    //!   - The packet is self-contained and does not reference external memory. Use DrawPacketBuilder to construct
    //!     an instance and either store in an RHI::Ptr or call 'delete' to release.
    //! 
    class DrawPacket final : public AZStd::intrusive_base
    {
        friend class DrawPacketBuilder;
        friend class UnitTest::DrawPacketTest;
        friend class UnitTest::MultiDeviceDrawPacketTest;

    public:
        using DrawItemVisitor = AZStd::function<void(DrawListTag, DrawItemProperties)>;

        //! Draw packets cannot be move constructed or copied, as they contain an additional memory payload.
        //! Use DrawPacketBuilder::Clone to copy a draw packet.
        AZ_DISABLE_COPY_MOVE(DrawPacket);

        //! Returns the mask representing all the draw lists affected by the packet.
        DrawListMask GetDrawListMask() const;

        //! Returns the number of draw items stored in the packet.
        size_t GetDrawItemCount() const;

        //! Returns the index associated with the given DrawListTag
        s32 GetDrawListIndex(DrawListTag drawListTag) const;

        //! Returns the DrawItem at the given index
        DrawItem* GetDrawItem(size_t index);

        //! Returns the DrawItem associated with the given DrawListTag
        DrawItem* GetDrawItem(DrawListTag drawListTag);

        //! Returns the draw item and its properties associated with the provided index.
        DrawItemProperties GetDrawItemProperties(size_t index) const;

        //! Returns the draw list tag associated with the provided index, used to filter the draw item into an appropriate pass.
        DrawListTag GetDrawListTag(size_t index) const;

        //! Returns the draw filter mask associated with the provided index, used to filter the draw item into an appropriate render pipeline.
        DrawFilterMask GetDrawFilterMask(size_t index) const;

        //! Overloaded operator delete for freeing a draw packet.
        void operator delete(void* p, size_t size);

        //! Update the root constant at the specified interval. The same root constants are shared by all draw items in the draw packet
        void SetRootConstant(uint32_t offset, const AZStd::span<uint8_t>& data);

        //! Set the instance count in all draw items.
        void SetInstanceCount(uint32_t instanceCount);

    private:
        /// Use DrawPacketBuilder to construct an instance.
        DrawPacket() = default;

        // The allocator used to release the memory when Release() is called.
        IAllocator* m_allocator = nullptr;

        // The bit-mask of all active filter tags.
        DrawListMask m_drawListMask = 0;

        // The index buffer view used when the draw call is indexed.
        IndexBufferView m_indexBufferView;

        uint8_t m_drawItemCount = 0;
        uint8_t m_streamBufferViewCount = 0;
        uint8_t m_shaderResourceGroupCount = 0;
        uint8_t m_uniqueShaderResourceGroupCount = 0;
        uint8_t m_rootConstantSize = 0;
        uint8_t m_scissorsCount = 0;
        uint8_t m_viewportsCount = 0;

        // List of draw items.
        DrawItem* m_drawItems = nullptr;

        // List of draw item sort keys associated with the draw item index.
        const DrawItemSortKey* m_drawItemSortKeys = nullptr;

        // List of draw list tags associated with the draw item index.
        const DrawListTag* m_drawListTags = nullptr;

        // List of draw filter masks associated with the draw item index.
        const DrawFilterMask* m_drawFilterMasks = nullptr;

        // List of shader resource groups shared by all draw items.
        const ShaderResourceGroup* const* m_shaderResourceGroups = nullptr;

        // List of shader resource groups that aren't shared, but unique for each draw item.
        const ShaderResourceGroup* const* m_uniqueShaderResourceGroups = nullptr;

        // List of inline constants shared by all draw items.
        const uint8_t* m_rootConstants = nullptr;

        // The list of stream buffer views. Each draw item has a view into the array.
        const StreamBufferView* m_streamBufferViews = nullptr;

        // Optional list of scissors to be used by all draw items.
        const Scissor* m_scissors = nullptr;

        // Optional list of viewports to be used by all draw items.
        const Viewport* m_viewports = nullptr;
    };
}
