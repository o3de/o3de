/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Model.h"

namespace ONNX
{
    //! Holds the digit that was inferenced and the time taken for a single inference run.
    //! Only used by MnistExample().
    struct MnistReturnValues
    {
        int64_t m_inference;
        float m_runtime;
    };

    //! Extension of ONNX Model used for Mnist example.
    //! Implements additional functionality useful to have for the example, such as keeping hold of the input and output vectors, and result
    //! (which the model doesn't do).
    struct MNIST
        : public Model
        , public AZ::TickBus::Handler
    {
    public:
        //! Loads an image from file into the correct format in m_input.
        //! @path is the file location of the image you want to inference (this NEEDS to be an 8-bit color depth png else it won't work).
        void LoadImage(const char* path);

        //! To be called after Model::Run(), uses softmax to get inference probabilities.
        //! Directly mutates m_output and m_result.
        std::ptrdiff_t GetResult();

        //! Invokes the correct setter function in ONNXBus, adding the value currently held in m_delta into an ImGui histogram group based
        //! on the m_modelName.
        void DispatchTimingSample();

        //! The MNIST dataset images are all 28 x 28 px, so you should probably be loading 28 x 28 images into the example.
        int m_imageWidth = 28;
        int m_imageHeight = 28;
        int m_imageSize = 784;

        std::vector<float> m_input; //!< This is the input that gets passed into Run(). A binary representation of the pixels in the image.
        std::vector<float> m_output; //!< This is the output that gets passed into Run().
        int64_t m_result{
            0
        }; //!< This will be the digit with the highest probability from the inference (what the model thinks the input number was).

    private:
        // Converts vector of output values into vector of probabilities.
        template<typename T>
        static void softmax(T& input);

        // Hook into gametick - used to run realtime inference demo.
        // The only thing that's in here is a call to Run() i.e. 1 inference run happens per tick.
        virtual void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    };

    //! This will run a single inference on the passed in MNIST instance.
    //! @mnist should be in a ready to run state, ie Load() should have been called.
    //! @path is the file location of the image you want to inference (this NEEDS to be an 8-bit color depth png else it won't work).
    //! Returns the inference digit and runtime.
    MnistReturnValues MnistExample(MNIST& mnist, const char* path);

    //! Runs through library of test mnist images in png format, calculating inference accuracy.
    //! @testsPerDigit specifies how many runs to do on each digit 0-9. Each run will be done on a unique image of that digit. Limit is
    //! ~5,000.
    //! @cudaEnable just specifies if the inferences should be run on gpu using CUDA or default cpu.
    void RunMnistSuite(int testsPerDigit, bool cudaEnable);
} // namespace ONNX
