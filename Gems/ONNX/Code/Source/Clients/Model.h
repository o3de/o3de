/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Timer.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <ONNX/ONNXBus.h>
#include <onnxruntime_cxx_api.h>

#include "upng/upng.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>

namespace ONNX
{
    //! Generic ONNX model which can be used to create an inference session and run inferences.
    class Model
    {
    public:
        Model()
        {
        }
        //! Required params to create session and run inference, passed to Load().
        struct InitSettings
        {
            //! Source of onnx model file.
            std::wstring m_modelFile = std::wstring{ W_GEM_ASSETS_PATH } + std::wstring{ L"/model.onnx" };
            std::string m_modelName = ""; //!< Used to create groupings for ImGui dashboard graphs in editor.
            std::vector<int64_t> m_inputShape; //!< Specifies dimensions of input.
            std::vector<int64_t> m_outputShape; //!< Specifies dimensions of output.
            bool m_cudaEnable = false; //!< Toggle to create a CUDA session on gpu, if disabled normal cpu session created.
        };
        //! Initialises necessary params in order to run inference.
        //! Must be run before Run() function.
        //! Creates the session, memory info, and extracts input and output names and count from onnx model file.
        //! Only needs to be run once, inferences using the same onnx model file can be run by providing different input/output params to
        //! Run().
        void Load(InitSettings& m_init_settings);

        //! Runs the inference using the loaded model.
        //! Input and output vectors are used to generate their respective tensors.
        //! Output is mutated directly.
        void Run(std::vector<float>& input, std::vector<float>& output);

        float m_delta; //!< Runtime in ms of latest inference.

    protected:
        bool m_cudaEnable;
        std::string m_modelName;
        AZ::Debug::Timer m_timer;
        Ort::MemoryInfo m_memoryInfo{ nullptr };
        Ort::Session m_session{ nullptr };
        std::vector<int64_t> m_inputShape;
        size_t m_inputCount;
        AZStd::vector<const char*> m_inputNames;
        std::vector<int64_t> m_outputShape;
        size_t m_outputCount;
        AZStd::vector<const char*> m_outputNames;
    };
} // namespace ONNX
