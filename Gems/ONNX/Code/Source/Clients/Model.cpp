/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Model.h"

namespace ONNX
{
    void Model::Load(InitSettings& initSettings)
    {
        // If no model name is provided, will default to the name of the onnx model file.
        if (initSettings.m_modelName.empty())
        {
            initSettings.m_modelName = std::filesystem::path(initSettings.m_modelFile).stem().string();
        }
        m_modelName = initSettings.m_modelName;

        // Grabs environment created on init of system component.
        Ort::Env* m_env;
        ONNXRequestBus::BroadcastResult(m_env, &ONNXRequestBus::Events::GetEnv);

        // OrtCudaProviderOptions must be added to the session options to specify execution on CUDA.
        // Can specify a number of parameters about the CUDA execution here - currently all left at default.
        Ort::SessionOptions sessionOptions;
        if (initSettings.m_cudaEnable)
        {
            OrtCUDAProviderOptions cuda_options;
            sessionOptions.AppendExecutionProvider_CUDA(cuda_options);
        }

        m_cudaEnable = initSettings.m_cudaEnable;
        m_session = Ort::Session::Session(*m_env, initSettings.m_modelFile.c_str(), sessionOptions);
        m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

        // Grabs memory allocator created on init of system component.
        Ort::AllocatorWithDefaultOptions* m_allocator;
        ONNXRequestBus::BroadcastResult(m_allocator, &ONNXRequestBus::Events::GetAllocator);

        m_inputShape = initSettings.m_inputShape;

        // Extract input and output names from model file and put into const char* vectors.
        m_inputCount = m_session.GetInputCount();
        for (size_t i = 0; i < m_inputCount; i++)
        {
            const char* in_name = m_session.GetInputName(i, *m_allocator);
            m_inputNames.push_back(in_name);
        }
        m_outputShape = initSettings.m_outputShape;
        m_outputCount = m_session.GetOutputCount();
        for (size_t i = 0; i < m_outputCount; i++)
        {
            const char* out_name = m_session.GetOutputName(i, *m_allocator);
            m_outputNames.push_back(out_name);
        }
    }

    void Model::Run(std::vector<float>& input, std::vector<float>& output)
    {
        m_timer.Stamp(); // Start timing of inference.

        // As far as I'm aware, there is no way of directly modifying the data of a tensor, so these must be initialised every time an
        // inference is run. Through testing this seems to be relatively lightweight with minimal performance impact.
        Ort::Value inputTensor =
            Ort::Value::CreateTensor<float>(m_memoryInfo, input.data(), input.size(), m_inputShape.data(), m_inputShape.size());
        Ort::Value outputTensor =
            Ort::Value::CreateTensor<float>(m_memoryInfo, output.data(), output.size(), m_outputShape.data(), m_outputShape.size());

        Ort::RunOptions runOptions;
        runOptions.SetRunLogVerbosityLevel(ORT_LOGGING_LEVEL_VERBOSE); // Gives more useful logging info if m_session.Run() fails.
        m_session.Run(runOptions, m_inputNames.data(), &inputTensor, m_inputCount, m_outputNames.data(), &outputTensor, m_outputCount);

        float delta = 1000 * m_timer.GetDeltaTimeInSeconds(); // Finish timing of inference and get time in milliseconds.
        m_delta = delta;
    }
} // namespace ONNX
