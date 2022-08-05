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
    static void MNIST::Softmax(T& input)
    {
        const float rowmax = *AZStd::ranges::max_element(input.begin(), input.end());
        AZStd::vector<float> y(input.size());
        float sum = 0.0f;
        for (size_t i = 0; i != input.size(); ++i)
        {
            sum += y[i] = AZStd::exp2(input[i] - rowmax);
        }
        for (size_t i = 0; i != input.size(); ++i)
        {
            input[i] = y[i] / sum;
        }
    }

    void MNIST::GetResult()
    {
        Softmax(m_output);
        m_result = AZStd::distance(m_output.begin(), AZStd::ranges::max_element(m_output.begin(), m_output.end()));
    }

    void MNIST::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        Run(m_input, m_output);
        DispatchTimingSample();
    }

    void MNIST::DispatchTimingSample()
    {
        // CPU and CUDA executions have different ImGui histogram groups, and so the inference data must be dispatched accordingly.
        if (m_cudaEnable)
        {
            ONNXRequestBus::Broadcast(&ONNXRequestBus::Events::AddTimingSampleCuda, m_modelName.c_str(), m_delta, m_modelColor);
        }
        else
        {
            ONNXRequestBus::Broadcast(&ONNXRequestBus::Events::AddTimingSample, m_modelName.c_str(), m_delta, m_modelColor);
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
        AZStd::vector<float> input(mnist.m_imageSize);
        mnist.m_input = input;
        AZStd::vector<float> output(10);
        mnist.m_output = output;

        MNIST::InitSettings modelInitSettings;
        modelInitSettings.m_inputShape = { 1, 1, 28, 28 };
        modelInitSettings.m_outputShape = { 1, 10 };

        if (cudaEnable)
        {
            modelInitSettings.m_modelName = "MNIST CUDA (Precomputed)";
            modelInitSettings.m_modelColor = AZ::Color::CreateFromRgba(56, 229, 59, 255);
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
        AZ::IO::FixedMaxPath mnistTestImageRoot;

        // This bit cycles through the folder with all the mnist test images, calling MnistExample() for the specified number of each digit.
        // The structure of the folder is as such: /testing/{digit}/{random_integer}.png    e.g /testing/3/10.png
        auto TestImage = [&mnist, &numOfCorrectInferences, &totalFiles, &totalRuntimeInMilliseconds, &mnistTestImageRoot, numOfEach](AZ::IO::Path digitFilePath, bool isFile) -> bool
        {
            if (!isFile)
            {
                AZ::IO::FixedMaxPath directoryName = digitFilePath.Filename();
                char* directoryEnd;
                AZ::s64 digit = strtoll(directoryName.c_str(), &directoryEnd, 10);
                if (directoryName.c_str() != directoryEnd)
                {
                    // How many files of that digit have been tested
                    int version = 0;

                    // The current folder is the @gemroot:ONNX@/testing/{digit} folder
                    // Search for any png files
                    auto RunMnistExample = [&mnist, &numOfCorrectInferences, &totalFiles, &totalRuntimeInMilliseconds, &mnistTestImageRoot, &directoryName, &version, &digit, &numOfEach](AZ::IO::Path pngFilePath, bool) -> bool
                    {
                        // Stop running examples once version limit for that digit has been reached
                        if ((version < numOfEach))
                        {
                            MnistReturnValues returnedValues = MnistExample(mnist, (mnistTestImageRoot / directoryName / pngFilePath).c_str());
                            if (returnedValues.m_inference == digit)
                            {
                                numOfCorrectInferences += 1;
                            }
                            mnist.DispatchTimingSample();
                            totalRuntimeInMilliseconds += returnedValues.m_runtime;
                            totalFiles++;
                            version++;
                        }
                        return true;
                    };
                    AZ::IO::SystemFile::FindFiles((mnistTestImageRoot / directoryName / "*.png").c_str(), RunMnistExample);
                }
            }
            return true;
        };

        // Get the FileIOBase to resolve the path to the ONNX gem
        AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
        if (fileIo->ResolvePath(mnistTestImageRoot, "@gemroot:ONNX@/Assets/mnist_png/testing"))
        {
            // onnxGemRoot is set to the root folder of the ONNX gem
            AZ::IO::SystemFile::FindFiles((mnistTestImageRoot / "*").c_str(), TestImage);
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

        AZ_Printf("ONNX", " Run Type: %s\n", cudaEnable ? "CUDA" : "CPU");
        AZ_Printf("ONNX", " Evaluated: %d  Correct: %d  Accuracy: %f%%\n", totalFiles, numOfCorrectInferences, accuracy);
        AZ_Printf("ONNX", " Total Runtime: %fms  Avg Runtime: %fms\n", totalRuntimeInMilliseconds, avgRuntimeInMilliseconds);
    }
} // namespace ONNX
