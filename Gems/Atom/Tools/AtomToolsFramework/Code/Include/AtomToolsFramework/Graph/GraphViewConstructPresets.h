/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <GraphCanvas/Types/ConstructPresets.h>
#endif

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/std/containers/map.h>

namespace AtomToolsFramework
{
    //! The implementation of the graph view requires construct presets in order to be able to create node groups and comment blocks.
    class GraphViewConstructPresets : public GraphCanvas::EditorConstructPresets
    {
    public:
        AZ_RTTI(GraphViewConstructPresets, "{5A4275ED-994D-403D-8571-7680D11106D4}", GraphCanvas::EditorConstructPresets);
        AZ_CLASS_ALLOCATOR(GraphViewConstructPresets, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        GraphViewConstructPresets() = default;
        ~GraphViewConstructPresets() override = default;
        void SetDefaultGroupPresets(const AZStd::map<AZStd::string, AZ::Color>& defaultGroupPresets);
        void InitializeConstructType(GraphCanvas::ConstructType constructType) override;

    private:
        // Default names and colors for node group presets
        AZStd::map<AZStd::string, AZ::Color> m_defaultGroupPresets;
    };
} // namespace AtomToolsFramework
