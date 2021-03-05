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

#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>
#include <SceneAPI/SceneCore/Utilities/HashHelper.h>
#include <cinttypes>

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
        SCENE_CORE_API void Write(const char* name, const AZStd::string& data);
        SCENE_CORE_API void Write(const char* name, double data);
        SCENE_CORE_API void Write(const char* name, uint64_t data);
        SCENE_CORE_API void Write(const char* name, int64_t data);
        SCENE_CORE_API void Write(const char* name, const DataTypes::MatrixType& data);
        SCENE_CORE_API void Write(const char* name, bool data);
        SCENE_CORE_API void Write(const char* name, Vector3 data);

        SCENE_CORE_API const AZStd::string& GetOutput() const;

    protected:
        AZStd::string m_output;
    };
}

#include "DebugOutput.inl"
