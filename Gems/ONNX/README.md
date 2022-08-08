# ONNX

This is an experimental gem implementing [ONNX Runtime](https://onnxruntime.ai/) in O3DE and demoing it using inference examples from the MNIST dataset with an Editor dashboard displaying inference statistics. Image decoding is done using a modified version of [uPNG](https://github.com/elanthis/upng).

![Image of ONNX Dashboard](https://user-images.githubusercontent.com/108667365/182392987-e39a38af-169e-47a8-b14d-c18c431386ee.png)

## Setup

1. Download the zip of the GPU version of ONNX Runtime v1.9.0 (Windows or Linux) from the [ONNX Runtime GitHub](https://github.com/microsoft/onnxruntime/releases/tag/v1.9.0), extract, and put the contents inside the *External/onnxruntime* folder in the gem. The path should look like *External/onnxruntime/\<include and lib folders in here\>*.
2. Put your *.onnx* file inside the *Assets* folder of the gem. By default the Model wrapper class looks for a file called *model.onnx*. To run the MNIST example you need an MNIST model, which you can get from the [ONNX GitHub](https://github.com/onnx/models/tree/main/vision/classification/mnist).
3. Download Johnathan Orsolini's [MNIST.png dataset](https://www.kaggle.com/datasets/playlist/mnistzip) from Kaggle, unzip (takes a while, there are a lot of pictures), and place inside the *Assets* folder of the gem (i.e. the path should look like *Assets/mnist_png/\<testing and training folders in here\>*).
4. Download the [uPNG source](https://github.com/elanthis/upng) from GitHub and copy into *Code/Source/Clients*. The source for uPNG has to be modified to work with the build, see [Modifying uPNG](#modifying-upng) below. You must have the following 2 files in these locations (the other uPNG files are unnecessary):
    - *Code/Source/Clients/upng/upng.c*
    - *Code/Source/Clients/upng/upng.h*
5. GPU inferencing using CUDA is ENABLED by default. Please see [Requirements to inference using GPU](#requirements-to-inference-using-gpu) below to make sure you have the correct dependencies installed. If you do not have a CUDA enabled NVIDIA GPU, or would like to run the gem without the CUDA example, then you must make sure that the definition `PUBLIC ENABLE_CUDA=true` on ***line 51*** in *Code/CMakeLists.txt* is either commented out or removed. When you do this, the Model class will inference using CPU regardless of params passed in.
6. Add the [ONNX](#onnx) gem to your project using the [Project Manager](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/) or the [Command Line Interface (CLI)](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/#using-the-command-line-interface-cli). See the documentation on  [Adding and Removing Gems in a Project](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/).
7. Compile your project and run.
8. Once the editor starts, and you go to edit a level with an initialized ONNX Model, you should be able to press the **HOME** (or equivalent) button on your keyboard to see the inference dashboard containing runtime graphs for any initialised model run history.
9. The MNIST example is located in the **ONNX.Tests** project, and demos the ONNX Model class by loading in the MNIST ONNX model file and running several thousand inferences against different digits in the MNIST testing dataset.

## Modifying uPNG:

1. Change the extension of *Code/Source/Clients/upng/upng.c* to *Code/Source/Clients/upng/upng.cpp*
2. Go into *Code/Source/Clients/upng/upng.cpp*
3. Modify ***lines 1172-1176*** as follows:
```diff
-	file = fopen(filename, "rb");
-	if (file == NULL) {
+   errno_t err = fopen_s(&file, filename, "rb");
+	if (err != NULL) {
		SET_ERROR(upng, UPNG_ENOTFOUND);
		return upng;
	}
```

## Requirements to inference using GPU:

- [CUDA Toolkit](https://developer.nvidia.com/cuda-toolkit) v11.4 or greater.
- [CUDNN library](https://developer.nvidia.com/cudnn) v8.2.4 or greater.
- [zlib](https://zlib.net/) v1.2.3 or greater.
