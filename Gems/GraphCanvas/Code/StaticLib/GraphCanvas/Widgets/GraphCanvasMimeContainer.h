/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <qstring.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>

#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>

namespace GraphCanvas
{
    class GraphCanvasMimeContainer
    {
    public:
        AZ_RTTI(GraphCanvasMimeContainer, "{CB8CAB35-B817-4910-AFC2-51881832591E}");
        AZ_CLASS_ALLOCATOR(GraphCanvasMimeContainer, AZ::SystemAllocator);        
        static void Reflect(AZ::ReflectContext* serializeContext);
        
        GraphCanvasMimeContainer() = default;
        virtual ~GraphCanvasMimeContainer();
        
        bool ToBuffer(AZStd::vector<char>& buffer);        
        bool FromBuffer(const char* data, AZStd::size_t size);
        bool FromBuffer(const AZStd::vector<char>& buffer);

        AZStd::vector< GraphCanvasMimeEvent* > m_mimeEvents;
    };
}
