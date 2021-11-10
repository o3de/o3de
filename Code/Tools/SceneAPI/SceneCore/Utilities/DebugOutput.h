/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>
#include <SceneAPI/SceneCore/Utilities/HashHelper.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <cinttypes>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace Events
        {
            struct ExportProduct;
            class ExportProductList;
        }
    }
}

namespace AZ::SceneAPI::Utilities
{
    class DebugOutput
    {
    public:
        template<typename T>
        void Write(const char* name, const AZStd::vector<T>& data);

        template<typename T>
        void Write(const char* name, const AZStd::vector<AZStd::vector<T>>& data);

        SCENE_CORE_API void Write(const char* name, const char* data);
        SCENE_CORE_API void WriteArray(const char* name, const unsigned int* data, int size);
        SCENE_CORE_API void Write(const char* name, const AZStd::string& data);
        SCENE_CORE_API void Write(const char* name, double data);
        SCENE_CORE_API void Write(const char* name, uint64_t data);
        SCENE_CORE_API void Write(const char* name, int64_t data);
        SCENE_CORE_API void Write(const char* name, const DataTypes::MatrixType& data);
        SCENE_CORE_API void Write(const char* name, bool data);
        SCENE_CORE_API void Write(const char* name, Vector3 data);
        SCENE_CORE_API void Write(const char* name, AZStd::optional<bool> data);
        SCENE_CORE_API void Write(const char* name, AZStd::optional<float> data);
        SCENE_CORE_API void Write(const char* name, AZStd::optional<AZ::Vector3> data);

        SCENE_CORE_API const AZStd::string& GetOutput() const;

        SCENE_CORE_API static void BuildDebugSceneGraph(const char* outputFolder, AZ::SceneAPI::Events::ExportProductList& productList, const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene, AZStd::string productName);

    protected:
        AZStd::string m_output;
    };
}

#include "DebugOutput.inl"
