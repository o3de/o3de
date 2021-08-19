# O3DE (Open 3D Engine)

O3DE (Open 3D Engine) is an open-source, real-time, multi-platform 3D engine that enables developers and content creators to build AAA games, cinema-quality 3D worlds, and high-fidelity simulations without any fees or commercial obligations.

## Contribute
For information about contributing to Open 3D Engine, visit https://o3de.org/docs/contributing/.

## Download and Install

This repository uses Git LFS for storing large binary files.  

Verify you have Git LFS installed by running the following command to print the version number.
```
git lfs --version 
```

If Git LFS is not installed, download and run the installer from: https://git-lfs.github.com/.

### Install Git LFS hooks 
```
git lfs install
```


### Clone the repository 

```shell
git clone https://github.com/o3de/o3de.git
```

## Building the Engine

### Build requirements and redistributables

#### Windows

*   Visual Studio 2019 16.9.2 minimum (All versions supported, including Community): [https://visualstudio.microsoft.com/downloads/](https://visualstudio.microsoft.com/downloads/)
    *   Install the following workloads:
        *   Game Development with C++
        *   MSVC v142 - VS 2019 C++ x64/x86
        *   C++ 2019 redistributable update
*   CMake 3.20 minimum: [https://cmake.org/download/](https://cmake.org/download/)

#### Optional

*   Wwise version 2021.1.1.7601 minimum: [https://www.audiokinetic.com/download/](https://www.audiokinetic.com/download/)
    *   Note: This requires registration and installation of a client application to download
    *   Note: It is generally okay to use a more recent version of Wwise, but some SDK updates will require code changes
    *   Make sure to select the `SDK(C++)` component during installation of Wwise
    *   CMake can find the Wwise install location in two ways:
        *   The `LY_WWISE_INSTALL_PATH` CMake cache variable -- this is checked first
        *   The `WWISEROOT` environment variable which is set when installing Wwise SDK

For more details and the latest requirements, refer to [System Requirements](https://o3de.org/docs/welcome-guide/requirements/) in the documentation.

### Quick start engine setup

To set up a project-centric source engine, complete the following steps. For other build options, refer to [Setting up O3DE from GitHub](https://o3de.org/docs/welcome-guide/setup/setup-from-github/) in the documentation.

1.  Create a writable folder to cache downloadable packages. You can also use this to store other redistributable SDKs.
    
1.  Install the following redistributables:
    - Visual Studio and VC++ redistributable can be installed to any location.
    - CMake can be installed to any location, as long as it's available in the system path.

1.  Configure the engine source into a solution using this command line, replacing `<your build path>`, `<your source path>`, and `<3rdParty cache path>` with the paths you've created:
    ```
    cmake -B <your build path> -S <your source path> -G "Visual Studio 16" -DLY_3RDPARTY_PATH=<3rdParty cache path> -DLY_UNITY_BUILD=ON
    ```
    
    Example:
    ```
    cmake -B C:\o3de\build\windows_vs2019 -S C:\o3de -G "Visual Studio 16" -DLY_3RDPARTY_PATH=C:\o3de-packages -DLY_UNITY_BUILD=ON
    ```
    
    > Note:  Do not use trailing slashes for the <3rdParty cache path>.

1.  Alternatively, you can do this through the CMake GUI:
    
    1.  Start `cmake-gui.exe`.
    1.  Select the local path of the repo under "Where is the source code".
    1.  Select a path where to build binaries under "Where to build the binaries".
    1.  Click **Configure**.
    1.  Wait for the key values to populate. Fill in the fields that are relevant, including `LY_3RDPARTY_PATH`.
    1.  Click **Generate**.
    
1.  Register the engine with this command:
    ```
    scripts\o3de.bat register --this-engine
    ```

1.  The configuration of the solution is complete. You are now ready to create a project and build the engine.

For more details on the steps above, refer to [Setting up O3DE from GitHub](https://o3de.org/docs/welcome-guide/setup/setup-from-github/) in the documentation.

### Setting up new projects and building the engine

1. From the O3DE repo folder, set up a new project using the `o3de create-project` command.
    ```
    scripts\o3de.bat create-project --project-path <your new project path>
    ```

1. Configure a solution for your project.
    ```
    cmake -B <your project build path> -S <your new project source path> -G "Visual Studio 16" -DLY_3RDPARTY_PATH=<3rdParty cache path> -DLY_UNITY_BUILD=ON
    ```

    Example:
    ```
    cmake -B C:\my-project\build\windows_vs2019 -S C:\my-project -G "Visual Studio 16" -DLY_3RDPARTY_PATH=C:\o3de-packages -DLY_UNITY_BUILD=ON
    ```
    
    > Note:  Do not use trailing slashes for the <3rdParty cache path>.

1. Build the project, Asset Processor, and Editor to binaries by running this command inside your project:
    ```
    cmake --build <your project build path> --target <New Project Name>.GameLauncher Editor --config profile -- /m
    ```

This will compile after some time and binaries will be available in the project build path you've specified, under `bin/profile`.

For a complete tutorial on project configuration, see [Creating Projects Using the Command Line Interface](https://o3de.org/docs/welcome-guide/create/creating-projects-using-cli/) in the documentation.

## License

For terms please see the LICENSE*.TXT files at the root of this distribution.
