/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/TypeInfo.h>

#include <AzCore/std/containers/unordered_set.h>

namespace AZ
{
    class Component;

    /**
     * Descriptor used when converting editor components to runtime components (slice processing, play-in-editor, etc).
     */
    struct ExportedComponent
    {
        AZ_TYPE_INFO(ExportedComponent, "{F8A00B8B-6981-4508-B939-731563849B97}");

        ExportedComponent()
            : m_component(nullptr)
            , m_deleteAfterExport(false) 
            , m_componentExportHandled(true)
        {}
        ExportedComponent(AZ::Component* component, bool deleteAfterExport, bool componentExportHandled = true)
            : m_component(component)
            , m_deleteAfterExport(deleteAfterExport)
            , m_componentExportHandled(componentExportHandled)
        {}

        AZ::Component* m_component;     ///< Pointer to exported component. Null is valid, and conveys no component should be exported.
        bool m_deleteAfterExport;       ///< If true (false by default), the returned component will be cleaned up by the asset pipeline.

        /**
         * If true (true by default), the component export has been handled.
         * This allows callbacks to announce whether they've handled or ignored the export.  If this has been set to false, anything set
         * in m_component or m_deleteAfterExport will be ignored.  If it has been set to true, the m_component value will be used as the
         * exported component.  (A value of null in m_component means "don't export anything")
         */
        bool m_componentExportHandled;
    };

    // List of platform tag Crcs for component exporting.
    using PlatformTagSet = AZStd::unordered_set<AZ::Crc32>;

    // Callback function delegate for customizing component export.
    using CustomExportCallbackFunc = AZStd::function<ExportedComponent(AZ::Component* thisComponent, const PlatformTagSet& tags)>;

} // namespace AZ
