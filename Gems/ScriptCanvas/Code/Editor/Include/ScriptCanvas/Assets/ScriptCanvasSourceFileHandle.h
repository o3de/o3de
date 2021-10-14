/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Uuid.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    class SourceHandle
    {
    public:
        AZ_TYPE_INFO(SourceHandle, "{65855A98-AE2F-427F-BFC8-69D45265E312}");
        AZ_CLASS_ALLOCATOR(SourceHandle, AZ::SystemAllocator, 0);

        SourceHandle() = default;

        SourceHandle(ScriptCanvas::DataPtr graph, const AZ::Uuid& id, AZStd::string_view path);

        void Clear();

        GraphPtrConst Get() const;

        const AZ::Uuid& Id() const;

        bool IsValid() const;

        GraphPtr Mod() const;

        operator bool() const;

        bool operator!() const;

        const AZStd::string& Path() const;
                
    private:
        ScriptCanvas::DataPtr m_data;
        AZ::Uuid m_id = AZ::Uuid::CreateNull();
        AZStd::string m_path;
    };
} 
