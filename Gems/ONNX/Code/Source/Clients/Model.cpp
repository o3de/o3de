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
    void Model::Load(const InitSettings& initSettings)
    {
        // Get the FileIOBase to resolve the path to the ONNX gem
        AZ::IO::FixedMaxPath onnxModelPath;

        // If no filepath provided for onnx model, set default to a model.onnx file in the Assets folder.
        if (initSettings.m_modelFile.empty())
        {
            AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
            fileIo->ResolvePath(onnxModelPath, "@gemroot:ONNX@/Assets/model.onnx");
        }
        else
        {
            onnxModelPath = initSettings.m_modelFile;
        }

        // If no model name is provided, will default to the name of the onnx model file.
        if (initSettings.m_modelName.empty())
        {
            AZ::StringFunc::Path::GetFileName(onnxModelPath.c_str(), m_modelName);
        }
        else
        {
            m_modelName = initSettings.m_modelName;
        }

        m_modelColor = initSettings.m_modelColor;

        // Grabs environment created on init of system component.
        Ort::Env* m_env;
        ONNXRequestBus::BroadcastResult(m_env, &ONNXRequestBus::Events::GetEnv);

#ifdef ENABLE_CUDA
        // OrtCudaProviderOptions must be added to the session options to specify execution on CUDA.
        // Can specify a number of parameters about the CUDA execution here - currently all left at default.
        Ort::SessionOptions sessionOptions;
        if (initSettings.m_cudaEnable)
        {
            OrtCUDAProviderOptions cuda_options;
            sessionOptions.AppendExecutionProvider_CUDA(cuda_options);
        }
        m_cudaEnable = initSettings.m_cudaEnable;
#endif

        // The model_path provided to Ort::Session needs to be const wchar_t*, even though the docs state const char* - doesn't work otherwise.
        AZStd::string onnxModelPathString = onnxModelPath.String();
        m_session = Ort::Session(*m_env, AZStd::wstring(onnxModelPathString.cbegin(), onnxModelPathString.cend()).c_str(), sessionOptions);
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

    void Model::Run(AZStd::vector<float>& input, AZStd::vector<float>& output)
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

        ONNXRequestBus::Broadcast(&::ONNX::ONNXRequestBus::Events::AddTimingSample, m_modelName.c_str(), m_delta, m_modelColor);
    }
} // namespace ONNX
