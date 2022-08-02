# ONNX

This is an experimental gem implementing [ONNX Runtime](https://onnxruntime.ai/) in O3DE and demoing it using inference examples from the MNIST dataset with an Editor dashboard displaying inference statistics. Image decoding is done using a modified version of [uPNG](https://github.com/elanthis/upng).

![Image of ONNX dashboard](Docs/Images/ImGuiDashboard.png)

## Setup

1. Download the zip of the GPU version of ONNX Runtime v1.9.0 for x64 Windows from the [ONNX Runtime GitHub](https://github.com/microsoft/onnxruntime/releases/tag/v1.9.0), extract, and put inside the *External* folder in the gem. The path should look like *External/onnxruntime-win-x64-gpu-1.9.0/\<include and lib folders in here\>*.
2. Put your *.onnx* file inside the *Assets* folder of the gem. By default the Model wrapper class looks for a file called *model.onnx*. To run the MNIST example you need an MNIST model, which you can get from the [ONNX GitHub](https://github.com/onnx/models/tree/main/vision/classification/mnist).
3. Download Johnathan Orsolini's [MNIST.png dataset](https://www.kaggle.com/datasets/playlist/mnistzip) from Kaggle, unzip (takes a while, there are a lot of pictures), and place inside the *Assets* folder of the gem (i.e. the path should look like *Assets/mnist_png/\<testing and training folders in here\>*).
4. Download the [uPNG source](https://github.com/elanthis/upng) from GitHub and copy into *Code/Source/Clients*. The source for uPNG has to be modified to work with the build, see [Modifying uPNG](#modifying-upng) below. You must have the following 2 files in these locations (the other uPNG files are unnecessary):
    - *Code/Source/Clients/upng/upng.c*
    - *Code/Source/Clients/upng/upng.h*
5. GPU inferencing using CUDA is ENABLED by default. Please see [Requirements to inference using GPU](#requirements-to-inference-using-gpu) below to make sure you have the correct dependencies installed. If you do not have a CUDA enabled NVIDIA GPU, or would like to run the demo without the CUDA example, then you must make sure that ***line 12*** in the top level *CMakeLists.txt* of the gem is set to `add_compile_definitions("ENABLE_CUDA=false")`.
6. Add the [ONNX](#onnx) gem to your project using the [Project Manager](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/) or the [Command Line Interface (CLI)](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/#using-the-command-line-interface-cli). See the documentation on  [Adding and Removing Gems in a Project](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/).
7. Compile your project and run.
8. Once the editor starts, and you go to edit a level, you should be able to press the **HOME** (or equivalent) button on your keyboard to see the inference dashboard as shown above.
    - The data shown above the graphs shows statistics for precomputed CPU inferences run before the editor starts up, on a selection of the MNIST testing images. The first (static) graph shows the individual runtimes for these inferences.
    - The data in the second graph shows the runtimes for the real-time CPU inferences being run on each game tick, which are run repeatedly on the same image.
    - If CUDA is enabled, a duplicate set of data and graphs will be shown for GPU inferences using CUDA.

## Modifying uPNG:

1. Go into *Code/Source/Clients/upng/upng.c*
2. Modify ***lines 1172-1176*** as follows:
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