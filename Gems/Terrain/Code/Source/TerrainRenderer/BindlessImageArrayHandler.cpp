/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/BindlessImageArrayHandler.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

namespace AZ::Render
{
    namespace
    {
        [[maybe_unused]] const char* BindlessImageArrayHandlerName = "TerrainFeatureProcessor";
    }

    void BindlessImageArrayHandler::Initialize(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& srg, const AZ::Name& propertyName)
    {
        if (!m_isInitialized && srg)
        {
            m_texturesIndex = srg->GetLayout()->FindShaderInputImageUnboundedArrayIndex(propertyName);
            if (m_texturesIndex.IsValid())
            {
                m_srg = srg;
                m_isInitialized = true;
            }
            else
            {
                AZ_Error(BindlessImageArrayHandlerName, m_texturesIndex.IsValid(), "Failed to find srg input constant %s.", propertyName.GetCStr());
            }
        }
        else
        {
            AZ_Error(BindlessImageArrayHandlerName, !m_isInitialized, "Already initialized.");
            AZ_Error(BindlessImageArrayHandlerName, srg, "Cannot initialize using a null shader resource group.");
        }
    }

    void BindlessImageArrayHandler::Reset()
    {
        m_srg = {};
        m_texturesIndex = {};
        m_isInitialized = false;
    }

    bool BindlessImageArrayHandler::IsInitialized() const
    {
        return m_isInitialized;
    }
    
    uint16_t BindlessImageArrayHandler::AppendBindlessImage(const AZ::RHI::ImageView* imageView)
    {
        uint16_t imageIndex = 0xFFFF;

        AZStd::unique_lock<AZStd::shared_mutex> lock(m_updateMutex);
        if (m_bindlessImageViewFreeList.size() > 0)
        {
            imageIndex = m_bindlessImageViewFreeList.back();
            m_bindlessImageViewFreeList.pop_back();
            m_bindlessImageViews.at(imageIndex) = imageView;
        }
        else
        {
            imageIndex = aznumeric_cast<uint16_t>(m_bindlessImageViews.size());
            m_bindlessImageViews.push_back(imageView);
        }
        return imageIndex;
    }

    void BindlessImageArrayHandler::UpdateBindlessImage(uint16_t index, const AZ::RHI::ImageView* imageView)
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_updateMutex);
        m_bindlessImageViews.at(index) = imageView;
    }

    void BindlessImageArrayHandler::RemoveBindlessImage(uint16_t index)
    {
        AZStd::unique_lock<AZStd::shared_mutex> lock(m_updateMutex);
        m_bindlessImageViews.at(index) = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::Magenta)->GetImageView();
        m_bindlessImageViewFreeList.push_back(index);
    }

    bool BindlessImageArrayHandler::UpdateSrg() const
    {
        if (!m_isInitialized)
        {
            AZ_Error("BindlessImageArrayHandler", false, "m_srgIndex must b")
            return false;
        }

        AZStd::array_view<const AZ::RHI::ImageView*> imageViews(m_bindlessImageViews.data(), m_bindlessImageViews.size());
        return m_srg->SetImageViewUnboundedArray(m_texturesIndex, imageViews);
    }

}
