/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>

namespace AzToolsFramework
{
    class InstanceDataNode;
}

namespace AtomToolsFramework
{
    //! Structure used as an opaque description of reflected objects contained within a document.
    //! An example use would be semi automatic handling of serialization or reflection to a property editor.
    struct DocumentObjectInfo final
    {
        bool m_visible = true;
        AZStd::string m_name;
        AZStd::string m_displayName;
        AZStd::string m_description;
        AZ::Uuid m_objectType = AZ::Uuid::CreateNull();
        void* m_objectPtr = {};
        AZStd::function<const char*(const AzToolsFramework::InstanceDataNode*)> m_nodeIndicatorFunction;
    };

    using DocumentObjectInfoVector = AZStd::vector<DocumentObjectInfo>;
} // namespace AtomToolsFramework
