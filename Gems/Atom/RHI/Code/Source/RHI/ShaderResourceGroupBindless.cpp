/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ShaderResourceGroupBindless.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ShaderResourceGroupData.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/ImageView.h>

namespace AZ::RHI
{
    void ShaderResourceGroupBindless::SetViews(
        ShaderInputBufferIndex indirectResourceBufferIndex,
        const RHI::BufferView* indirectResourceBuffer,
        AZStd::span<const ImageView* const> imageViews,
        uint32_t* outIndices,
        bool viewReadOnly,
        uint32_t arrayIndex)
    {
        auto key = AZStd::make_pair(indirectResourceBufferIndex, arrayIndex);
        auto it = m_bindlessResourceViews.find(key);
        if (it == m_bindlessResourceViews.end())
        {
            it = m_bindlessResourceViews.try_emplace(key).first;
        }
        else
        {
            // Release existing views
            it->second.m_resources.clear();
        }

        size_t i = 0;
        for (const ImageView* imageView : imageViews)
        {
            it->second.m_resources.push_back(imageView);

            if (viewReadOnly)
            {
                outIndices[i] = imageView->GetBindlessReadIndex();
            }
            else
            {
                outIndices[i] = imageView->GetBindlessReadWriteIndex();
            }
            ++i;
        }

        m_parent->SetBufferView(indirectResourceBufferIndex, indirectResourceBuffer);
    }

    void ShaderResourceGroupBindless::SetViews(
        ShaderInputBufferIndex indirectResourceBufferIndex,
        const RHI::BufferView* indirectResourceBuffer,
        AZStd::span<const BufferView* const> bufferViews,
        uint32_t* outIndices,
        bool viewReadOnly,
        uint32_t arrayIndex)
    {
        auto key = AZStd::make_pair(indirectResourceBufferIndex, arrayIndex);
        auto it = m_bindlessResourceViews.find(key);
        if (it == m_bindlessResourceViews.end())
        {
            it = m_bindlessResourceViews.try_emplace(key).first;
        }
        else
        {
            // Release existing views
            it->second.m_resources.clear();
        }

        size_t i = 0;
        for (const BufferView* bufferView : bufferViews)
        {
            it->second.m_resources.push_back(bufferView);

            if (viewReadOnly)
            {
                outIndices[i] = bufferView->GetBindlessReadIndex();
            }
            else
            {
                outIndices[i] = bufferView->GetBindlessReadWriteIndex();
            }
            ++i;
        }

        m_parent->SetBufferView(indirectResourceBufferIndex, indirectResourceBuffer);
    }

} // namespace AZ::RHI
