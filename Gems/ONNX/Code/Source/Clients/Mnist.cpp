/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Mnist.h"

namespace ONNX
{
    template<typename T>
    static void MNIST::softmax(T& input)
    {
        float rowmax = *std::max_element(input.begin(), input.end());
        std::vector<float> y(input.size());
        float sum = 0.0f;
        for (size_t i = 0; i != input.size(); ++i)
        {
            sum += y[i] = std::exp(input[i] - rowmax);
        }
        for (size_t i = 0; i != input.size(); ++i)
        {
            input[i] = y[i] / sum;
        }
    }

    std::ptrdiff_t MNIST::GetResult()
    {
        softmax(m_output);
        m_result = std::distance(m_output.begin(), std::max_element(m_output.begin(), m_output.end()));
        return m_result;
    }

    void MNIST::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        Run(m_input, m_output);
        DispatchTimingSample();
    }

    void MNIST::DispatchTimingSample()
    {
        // Ignore any unusually large inference times - this is mostly an issue with the first inference on CUDA, where the average runtimes
        // and ImGui graphs are distorted by anomalies.
        if (m_delta < 10.0f)
        {
            // CPU and CUDA executions have different ImGui histogram groups, and so the inference data must be dispatched accordingly.
            if (m_cudaEnable)
            {
                ONNXRequestBus::Broadcast(&ONNXRequestBus::Events::AddTimingSampleCuda, m_modelName.c_str(), m_delta);
            }
            else
            {
                ONNXRequestBus::Broadcast(&ONNXRequestBus::Events::AddTimingSample, m_modelName.c_str(), m_delta);
            }
        }
    }

    void MNIST::LoadImage(const char* path)
    {
        // Gets the png image from file and decodes using upng library.
        upng_t* upng = upng_new_from_file(path);
        upng_decode(upng);
        const unsigned char* buffer = upng_get_buffer(upng);

        // Converts image from buffer into binary greyscale representation.
        // i.e. a pure black pixel is a 0, anything else is a 1.
        // Bear in mind that the images in the dataset are flipped compared to how we'd usually think,
        // so the background is black and the actual digit is white.
        for (int y = 0; y < m_imageHeight; y++)
        {
            for (int x = 0; x < m_imageWidth; x++)
            {
                int content = static_cast<int>(buffer[(y)*m_imageWidth + x]);
                if (content == 0)
                {
                    m_input[m_imageWidth * y + x] = 0.0f;
                }
                else
                {
                    m_input[m_imageHeight * y + x] = 1.0f;
                }
            }
        }
    }

    MnistReturnValues MnistExample(MNIST& mnist, const char* path)
    {
        mnist.LoadImage(path);
        mnist.Run(mnist.m_input, mnist.m_output);
        mnist.GetResult();

        MnistReturnValues returnValues;
        returnValues.m_inference = mnist.m_result;
        returnValues.m_runtime = mnist.m_delta;
        return (returnValues);
    }

    void RunMnistSuite(int testsPerDigit, bool cudaEnable)
    {
        // Initialises and loads the mnist model.
        // The same instance of the model is used for all runs.
        MNIST mnist;
        std::vector<float> input(mnist.m_imageSize);
        mnist.m_input = input;
        std::vector<float> output(10);
        mnist.m_output = output;

        MNIST::InitSettings modelInitSettings;
        modelInitSettings.m_inputShape = { 1, 1, 28, 28 };
        modelInitSettings.m_outputShape = { 1, 10 };

        if (cudaEnable)
        {
            modelInitSettings.m_modelName = "MNIST CUDA (Precomputed)";
            modelInitSettings.m_cudaEnable = true;
        }
        else
        {
            modelInitSettings.m_modelName = "MNIST (Precomputed)";
        }

        mnist.Load(modelInitSettings);

        int numOfEach = testsPerDigit;
        int totalFiles = 0;
        int64_t numOfCorrectInferences = 0;
        float totalRuntimeInMilliseconds = 0;

        // This bit cycles through the folder with all the mnist test images, calling MnistExample() for the specified number of each digit.
        // The structure of the folder is as such: /testing/{digit}/{random_integer}.png    e.g /testing/3/10.png
        for (int digit = 0; digit < 10; digit++)
        {
            std::filesystem::directory_iterator iterator =
                std::filesystem::directory_iterator{ std::wstring{ W_GEM_ASSETS_PATH } + L"/mnist_png/testing/" + std::to_wstring(digit) + L"/" };
            for (int version = 0; version < numOfEach; version++)
            {
                std::string filepath = iterator->path().string();
                MnistReturnValues returnedValues = MnistExample(mnist, filepath.c_str());

                // Ignore any unusually large inference times (>10ms) - this is mostly an issue with the first inference on CUDA, where the
                // average runtimes and ImGui graphs are distorted by anomalies.
                if (returnedValues.m_runtime < 10.0f)
                {
                    if (returnedValues.m_inference == digit)
                    {
                        numOfCorrectInferences += 1;
                    }
                    mnist.DispatchTimingSample();
                    totalRuntimeInMilliseconds += returnedValues.m_runtime;
                    iterator++;
                    totalFiles++;
                }
            }
        }

        float accuracy = ((float)numOfCorrectInferences / (float)totalFiles) * 100.0f;
        float avgRuntimeInMilliseconds = totalRuntimeInMilliseconds / (totalFiles);

        // The stats for the run are broadcast to their respective timing data members in the system component.
        // This data is used to populate the header in the ImGui dashboard in the editor.
        if (cudaEnable)
        {
            ONNXRequestBus::Broadcast(
                &ONNXRequestBus::Events::SetPrecomputedTimingDataCuda,
                totalFiles,
                numOfCorrectInferences,
                totalRuntimeInMilliseconds,
                avgRuntimeInMilliseconds);
        }
        else
        {
            ONNXRequestBus::Broadcast(
                &ONNXRequestBus::Events::SetPrecomputedTimingData,
                totalFiles,
                numOfCorrectInferences,
                totalRuntimeInMilliseconds,
                avgRuntimeInMilliseconds);
        }

        AZ_Printf("\nONNX", " Run Type: %s", cudaEnable ? "CUDA" : "CPU");
        AZ_Printf("\nONNX", " Evaluated: %d  Correct: %d  Accuracy: %f%%", totalFiles, numOfCorrectInferences, accuracy);
        AZ_Printf("\nONNX", " Total Runtime: %fms  Avg Runtime: %fms", totalRuntimeInMilliseconds, avgRuntimeInMilliseconds);
    }
} // namespace ONNX
