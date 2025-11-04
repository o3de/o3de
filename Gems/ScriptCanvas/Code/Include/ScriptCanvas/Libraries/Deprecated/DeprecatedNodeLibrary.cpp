/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DeprecatedNodeLibrary.h"

#include <AzCore/Serialization/EditContext.h>

#include <Libraries/Libraries.h>

namespace ScriptCanvas
{
    namespace DeprecatedLibrary
    {
        REFLECT_GENERIC_FUNCTION_NODE(Entity, "{C9789111-6A18-4C6E-81A7-87D057D59D2C}");
        REFLECT_GENERIC_FUNCTION_NODE(World, "{55A54AF1-B545-4C12-9F74-01D30789CA1D}");
        REFLECT_GENERIC_FUNCTION_NODE(AABB, "{AB0C2753-680E-47AD-8277-66B3AC01C659}");
        REFLECT_GENERIC_FUNCTION_NODE(Color, "{6CC7B2F9-F551-4CB8-A713-4149959AE337}");
        REFLECT_GENERIC_FUNCTION_NODE(CRC, "{D8B65A5B-38FF-4B41-9FA4-FCA080D75625}");
        REFLECT_GENERIC_FUNCTION_NODE(Math, "{76898795-2B30-4645-B6D4-67568ECC889F}");
        REFLECT_GENERIC_FUNCTION_NODE(Matrix3x3, "{8C5F6959-C2C4-46D9-9FCD-4DC234E7732D}");
        REFLECT_GENERIC_FUNCTION_NODE(Matrix4x4, "{537AB179-E23C-492D-8EF7-53845A2DB163}");
        REFLECT_GENERIC_FUNCTION_NODE(OBB, "{8CBDA3B7-9DCD-4A04-A12C-123C322CA63A}");
        REFLECT_GENERIC_FUNCTION_NODE(Plane, "{F2C799DF-2CC1-4DD8-91DC-18C2D517BAB0}");
        REFLECT_GENERIC_FUNCTION_NODE(Quaternion, "{9BE75E2E-AA07-4767-9BF3-905C289EB38A}");
        REFLECT_GENERIC_FUNCTION_NODE(Random, "{D9DF1385-6C5C-4E41-94CA-8B10E9D8FAAF}");
        REFLECT_GENERIC_FUNCTION_NODE(String, "{5B700838-21A2-4579-9303-F4A4822AFEF4}");
        REFLECT_GENERIC_FUNCTION_NODE(Transform, "{59E0EF87-352C-4CFB-A810-5B8752BDD1EF}");
        REFLECT_GENERIC_FUNCTION_NODE(Vector2, "{FD1BFADF-BA1F-4AFC-819B-ABA1F22AE6E6}");
        REFLECT_GENERIC_FUNCTION_NODE(Vector3, "{53EA7604-E3AE-4E88-BE0E-66CBC488A7DB}");
        REFLECT_GENERIC_FUNCTION_NODE(Vector4, "{66027D7A-FCD1-4592-93E5-EB4B7F4BD671}");
    } // namespace Math

    void DeprecatedNodeLibrary::Reflect(AZ::ReflectContext*)
    {        
    }

    AZStd::vector<AZ::ComponentDescriptor*> DeprecatedNodeLibrary::GetComponentDescriptors()
    {
        AZStd::vector<AZ::ComponentDescriptor*> descriptors = {};

        return descriptors;
    }
} // namespace ScriptCanvas
