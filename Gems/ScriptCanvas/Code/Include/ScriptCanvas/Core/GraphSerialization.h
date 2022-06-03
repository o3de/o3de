/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/GraphData.h>

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvas
{
    class ScriptCanvasData;

    class SourceTree
    {
    public:
        AZ_CLASS_ALLOCATOR(SourceTree, AZ::SystemAllocator, 0);

        SourceTree* m_parent = nullptr;
        SourceHandle m_source;
        AZStd::vector<SourceTree> m_dependencies;

        SourceTree* ModRoot();
        void SetParent(SourceTree& parent);
        AZStd::string ToString(size_t depth = 0) const;
    };

    enum class MakeInternalGraphEntitiesUnique
    {
        No,
        Yes,
    };

    struct DeserializeResult
    {
        bool m_isSuccessful = false;
        AZStd::string m_jsonResults;
        AZStd::string m_errors;
        DataPtr m_graphDataPtr;

        inline operator bool() const
        {
            return m_isSuccessful;
        }

        inline bool operator!() const
        {
            return !m_isSuccessful;
        }
    };

    DeserializeResult Deserialize
        ( AZStd::string_view source
        , MakeInternalGraphEntitiesUnique makeUniqueEntities = MakeInternalGraphEntitiesUnique::Yes);

    struct SerializationResult
    {
        bool m_isSuccessful;
        AZStd::string m_errors;
        
        inline operator bool() const
        {
            return m_isSuccessful;
        }

        inline bool operator!() const
        {
            return !m_isSuccessful;
        }
    };

    SerializationResult Serialize(const ScriptCanvasData& graphData, AZ::IO::GenericStream& stream);
}
