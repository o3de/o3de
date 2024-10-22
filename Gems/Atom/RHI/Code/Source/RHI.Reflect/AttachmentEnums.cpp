/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    HardwareQueueClassMask GetHardwareQueueClassMask(HardwareQueueClass hardwareQueueClass)
    {
        return static_cast<HardwareQueueClassMask>(AZ_BIT(static_cast<uint32_t>(hardwareQueueClass)));
    }

    const char* GetHardwareQueueClassName(HardwareQueueClass hardwareQueueClass)
    {
        AZStd::array<const char*, 3> nameTable =
        {{
            "Graphics",
            "Compute",
            "Copy"
        }};
        return nameTable[static_cast<size_t>(hardwareQueueClass)];
    }

    HardwareQueueClass GetMostCapableHardwareQueue(HardwareQueueClassMask queueMask)
    {
        if (CheckBitsAny(queueMask, HardwareQueueClassMask::Graphics))
        {
            return HardwareQueueClass::Graphics;
        }
        else if (CheckBitsAny(queueMask, HardwareQueueClassMask::Compute))
        {
            return HardwareQueueClass::Compute;
        }
        else
        {
            return HardwareQueueClass::Copy;
        }
    }

    bool IsHardwareQueueMoreCapable(HardwareQueueClass queueA, HardwareQueueClass queueB)
    {
        return queueA < queueB;
    }

    const char* ToString(ScopeAttachmentAccess attachmentAccess)
    {
        switch (attachmentAccess)
        {
        case ScopeAttachmentAccess::Read:
            return "Read";
        case ScopeAttachmentAccess::ReadWrite:
            return "ReadWrite";
        case ScopeAttachmentAccess::Write:
            return "Write";
        default:
            AZ_Assert(false, "Unkown ScopeAttachmentAccess: %d", static_cast<uint32_t>(attachmentAccess));
            return "Unknown";
        }
    }

    const char* ToString(ScopeAttachmentUsage attachmentUsage)
    {
        switch (attachmentUsage)
        {
        case ScopeAttachmentUsage::RenderTarget:
            return "RenderTarget";
        case ScopeAttachmentUsage::DepthStencil:
            return "DepthStencil";
        case ScopeAttachmentUsage::SubpassInput:
            return "SubpassInput";
        case ScopeAttachmentUsage::Shader:
            return "Shader";
        case ScopeAttachmentUsage::Copy:
            return "Copy";
        case ScopeAttachmentUsage::Resolve:
            return "Resolve";
        case ScopeAttachmentUsage::Predication:
            return "Predication";
        case ScopeAttachmentUsage::Indirect:
            return "Indirect";
        case ScopeAttachmentUsage::InputAssembly:
            return "InputAssembly";
        case ScopeAttachmentUsage::ShadingRate:
            return "ShadingRate";
        case ScopeAttachmentUsage::Uninitialized:
            return "Uninitialized";
        default:
            AZ_Assert(false, "Unkown ScopeAttachmentUsage: %d", static_cast<uint32_t>(attachmentUsage));
            return "Unknown";
        }
    }

    AZStd::string ToString(ScopeAttachmentStage attachmentStage)
    {
        if (attachmentStage == ScopeAttachmentStage::Uninitialized)
        {
            return "Uninitialized";
        }

        AZStd::string stages;
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::VertexShader))
        {
            stages += "VertexShader|";
        }
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::FragmentShader))
        {
            stages += "FragmentShader|";
        }
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::ComputeShader))
        {
            stages += "ComputeShader|";
        }
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::RayTracingShader))
        {
            stages += "RayTracingShader|";
        }
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::EarlyFragmentTest))
        {
            stages += "EarlyFragmentTest|";
        }
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::LateFragmentTest))
        {
            stages += "LateFragmentTest|";
        }
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::ColorAttachmentOutput))
        {
            stages += "ColorAttachmentOutput|";
        }
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::Copy))
        {
            stages += "Copy|";
        }
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::Predication))
        {
            stages += "Predication|";
        }
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::DrawIndirect))
        {
            stages += "DrawIndirect|";
        }
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::VertexInput))
        {
            stages += "VertexInput|";
        }
        if (CheckBitsAll(attachmentStage, ScopeAttachmentStage::ShadingRate))
        {
            stages += "ShadingRate|";
        }

        if (!stages.empty())
        {
            stages.pop_back();
        }

        return stages;
    }

    const char* ToString(ScopeAttachmentUsage usage, ScopeAttachmentAccess acess)
    {
        switch (usage)
        {
        case ScopeAttachmentUsage::RenderTarget:
            return "RenderTarget";

        case ScopeAttachmentUsage::DepthStencil:
            return CheckBitsAny(acess, ScopeAttachmentAccess::Write) ? "DepthStencilReadWrite" : "DepthStencilRead";

        case ScopeAttachmentUsage::SubpassInput:
            return "SubpassInput";

        case ScopeAttachmentUsage::Shader:
            return CheckBitsAny(acess, ScopeAttachmentAccess::Write) ? "ShaderReadWrite" : "ShaderRead";

        case ScopeAttachmentUsage::Copy:
            return CheckBitsAny(acess, ScopeAttachmentAccess::Write) ? "CopyDest" : "CopySource";

        case ScopeAttachmentUsage::Predication:
            return "Predication";

        case ScopeAttachmentUsage::InputAssembly:
            return "InputAssembly";

        case ScopeAttachmentUsage::ShadingRate:
            return "ShadingRate";

        case ScopeAttachmentUsage::Resolve:
            return "Resolve";

        case ScopeAttachmentUsage::Indirect:
            return "Indirect";

        case ScopeAttachmentUsage::Uninitialized:
            return "Uninitialized";
        }

        return "Unknown";
    }

    ScopeAttachmentAccess AdjustAccessBasedOnUsage(ScopeAttachmentAccess access, ScopeAttachmentUsage usage)
    {
        switch (usage)
        {
        // Remap read/write to write for Color scope attachments. This is because from a user standpoint, an attachment
        // might be an input/output to a pass (which maps to read/write) but still be used as a render target (write).
        case ScopeAttachmentUsage::RenderTarget:
            if (access == ScopeAttachmentAccess::ReadWrite)
            {
                return ScopeAttachmentAccess::Write;
            }
            return access;

        // Remap read/write to write for DepthStencil scope attachments. This is because from a user standpoint, an attachment
        // might be an input/output to a pass (which maps to read/write) but still be used as a render target (write).
        case ScopeAttachmentUsage::DepthStencil:
            if (access == ScopeAttachmentAccess::ReadWrite)
            {
                return ScopeAttachmentAccess::Write;
            }
            return access;

        // Remap read/write to read for Subpass input scope attachments.
        // We disallow write access and throw an error because having a write access on an subpass input attachment is nonsensical.
        case ScopeAttachmentUsage::SubpassInput:
            AZ_Error("ScopeAttachment", access == ScopeAttachmentAccess::Read, "ScopeAttachmentAccess cannot be 'Write' when usage is 'SubpassInput'.");
            return ScopeAttachmentAccess::Read;

        // Remap write to read/write for Shader scope attachments. This is because  
        // a write Shader scope is a UAV under the hood, and UAVs are read/write.
        case ScopeAttachmentUsage::Shader:
            if (access == ScopeAttachmentAccess::Write)
            {
                return ScopeAttachmentAccess::ReadWrite;
            }
            return access;

        // Read/write access for Copy scope attachments can happen when copying between two devices.
        case ScopeAttachmentUsage::Copy:
            return access;

        case ScopeAttachmentUsage::InputAssembly:
            AZ_Error("ScopeAttachment", !RHI::CheckBitsAll(access, ScopeAttachmentAccess::Write), "ScopeAttachmentAccess cannot be 'Write' when usage is 'InputAssembly'.");
            return ScopeAttachmentAccess::Read;

        // Remap read/write to read for ShadingRate scope attachments.
        // We disallow write access and throw an error because having a write access on an ShadingRate input attachment is not allowed.
        case ScopeAttachmentUsage::ShadingRate:
            AZ_Error(
                "ScopeAttachment",
                access == ScopeAttachmentAccess::Read,
                "ScopeAttachmentAccess cannot be 'Write' when usage is 'ShadingRate'.");
            return ScopeAttachmentAccess::Read;

        // No access adjustment for Resolve or Predication
        case ScopeAttachmentUsage::Resolve:
            return access;
        case ScopeAttachmentUsage::Predication:
            return access;

        case ScopeAttachmentUsage::Indirect:
            AZ_Error(
                "ScopeAttachment",
                !RHI::CheckBitsAll(access, ScopeAttachmentAccess::Write),
                "ScopeAttachmentAccess cannot be 'Write' when usage is 'Indirect'.");
            return ScopeAttachmentAccess::Read;

        case ScopeAttachmentUsage::Uninitialized:
            return access;
        default:
            AZ_Assert(false, "Unkown ScopeAttachmentUsage: %d", static_cast<uint32_t>(usage));
            return access;
        }
    }

    const char* ToString(AZ::RHI::AttachmentType attachmentType)
    {
        switch (attachmentType)
        {
        case AZ::RHI::AttachmentType::Image:
            return "Image";
        case AZ::RHI::AttachmentType::Buffer:
            return "Buffer";
        case AZ::RHI::AttachmentType::Resolve:
            return "Resolve";
        case AZ::RHI::AttachmentType::Uninitialized:
        default:
            return "Uninitialized";
        }
    }

    const char* ToString(HardwareQueueClass hardwareClass)
    {
        switch (hardwareClass)
        {
        case HardwareQueueClass::Graphics:
            return "Graphics";
        case HardwareQueueClass::Compute:
            return "Compute";
        case HardwareQueueClass::Copy:
            return "Copy";
        default:
            return "Invalid";
        }
    }
}
