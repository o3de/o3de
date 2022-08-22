"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class VulkanValidationErrors:
    identifiers = ["vkDebugMessage"]

    # List to add vulkan validation errors that will be ignored.
    errors_to_skip = [
        #"VUID-VkDescriptorImageInfo-imageLayout-00344", # GHI-xxx
        #"VUID-vkCmdDraw-None-02699", # GHI-xxx
        #"VUID-VkMemoryAllocateInfo-flags-03331", # GHI-xxx
        #"UNASSIGNED-CoreValidation-Shader-InconsistentSpirv"  # GHI-xxx
        ]

    def CheckError(error_id, error_msg):
        if error_id in VulkanValidationErrors.identifiers:
            for error_to_skip in VulkanValidationErrors.errors_to_skip:
                if error_to_skip in error_msg:
                    return False # Skip error
            return True # Found error
        return False # Not a Vulkan error
        