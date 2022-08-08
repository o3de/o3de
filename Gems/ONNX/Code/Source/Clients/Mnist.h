/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/SystemFile.h>

#include "Model.h"
#include "upng/upng.h"

namespace Mnist
{
    //! Holds the digit that was inferenced and the time taken for a single inference run.
    //! Only used by MnistExample().
    struct MnistReturnValues
    {
        int64_t m_inference;
        float m_runtime;
    };

    //! Holds the data gathered from RunMnistSuite(), which tests the MNIST ONNX model against images from the MNIST dataset.
    struct InferenceData
    {
        float m_totalRuntimeInMs;
        float m_averageRuntimeInMs;
        int m_totalNumberOfInferences;
        int64_t m_numberOfCorrectInferences;
    };

    //! Extension of ONNX Model used for Mnist example.
    //! Implements additional functionality useful to have for the example, such as keeping hold of the input and output vectors, and result
    //! (which the model doesn't do).
    struct Mnist
        : public ::ONNX::Model
    {
    public:
        //! Loads an image from file into the correct format in m_input.
        //! @path is the file location of the image you want to inference (this NEEDS to be an 8-bit color depth png else it won't work).
        void LoadImage(const char* path);

        //! To be called after Model::Run(), uses softmax to get inference probabilities.
        //! Directly mutates m_output and m_result.
        void GetResult();

        //! The MNIST dataset images are all 28 x 28 px, so you should probably be loading 28 x 28 images into the example.
        int m_imageWidth = 28;
        int m_imageHeight = 28;
        int m_imageSize = 784;

        AZStd::vector<float> m_input; //!< This is the input that gets passed into Run(). A binary representation of the pixels in the image.
        AZStd::vector<float> m_output; //!< This is the output that gets passed into Run().
        int64_t m_result{
            0
        }; //!< This will be the digit with the highest probability from the inference (what the model thinks the input number was).

    private:
        // Converts vector of output values into vector of probabilities.
        template<typename T>
        static void Softmax(T& input);
    };

    //! This will run a single inference on the passed in MNIST instance.
    //! @mnist should be in a ready to run state, ie Load() should have been called.
    //! @path is the file location of the image you want to inference (this NEEDS to be an 8-bit color depth png else it won't work).
    //! Returns the inference digit and runtime.
    MnistReturnValues MnistExample(Mnist& mnist, const char* path);

    //! Runs through library of test mnist images in png format, calculating inference accuracy.
    //! @testsPerDigit specifies how many runs to do on each digit 0-9. Each run will be done on a unique image of that digit. Limit is
    //! ~9,000.
    //! @cudaEnable just specifies if the inferences should be run on gpu using CUDA or default cpu.
    InferenceData RunMnistSuite(int testsPerDigit, bool cudaEnable);
} // namespace Mnist
