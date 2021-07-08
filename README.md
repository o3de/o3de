# Open 3D Engine

Open 3D Engine (O3DE) is an open-source, real-time, multi-platform 3D engine that enables developers and content creators to build AAA games, cinema-quality 3D worlds, and high-fidelity simulations without any fees or commercial obligations.

## Contribute
For information about contributing to Open 3D Engine, visit https://o3de.org/docs/contributing/

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
### Build Requirements and redistributables
#### Windows

*   Visual Studio 2019 16.9.2 minimum (All versions supported, including Community): [https://visualstudio.microsoft.com/downloads/](https://visualstudio.microsoft.com/downloads/)
    *   Install the following workloads:
        *   Game Development with C++
        *   MSVC v142 - VS 2019 C++ x64/x86
        *   C++ 2019 redistributable update
*   CMake 3.20 minimum: [https://cmake.org/download/](https://cmake.org/download/)

#### Optional

*   Wwise - 2021.1.1.7601 minimum: [https://www.audiokinetic.com/download/](https://www.audiokinetic.com/download/)
    *   Note: This requires registration and installation of a client application to download
    *   Make sure to select the SDK(C++) component during installation of Wwise
    *   You will also need to set an environment variable: `set LY_WWISE_INSTALL_PATH=<path to Wwise version>`
    *   For example: `set LY_WWISE_INSTALL_PATH="C:\Program Files (x86)\Audiokinetic\Wwise 2021.1.1.7601"`

### Quick Start Build Steps

1.  Create a writable folder to cache 3rd Party dependencies. You can also use this to store other redistributable SDKs.
    
1.  Install the following redistributables to the following:
    - Visual Studio and VC++ redistributable can be installed to any location
    - CMake can be installed to any location, as long as it's available in the system path
    - (Optional) Wwise can be installed anywhere, but you will need to set an environment variable for CMake to detect it:  `set LY_WWISE_INSTALL_PATH=<path to Wwise>`
    
1.  Configure the source into a solution using this command line, replacing <your build path> and <3rdParty cache path> to a path you've created:
    ```
    cmake -B <your build path> -S <your source path> -G "Visual Studio 16" -DLY_3RDPARTY_PATH=<3rdParty cache path> -DLY_UNITY_BUILD=ON -DLY_PROJECTS=AutomatedTesting 
    ```
    > Note:  Do not use trailing slashes for the <3rdParty cache path>

1.  Alternatively, you can do this through the CMake GUI:
    
    1.  Start `cmake-gui.exe`
    1.  Select the local path of the repo under "Where is the source code"
    1.  Select a path where to build binaries under "Where to build the binaries"
    1.  Click "Configure"
    1.  Wait for the key values to populate. Fill in the fields that are relevant, including `LY_3RDPARTY_PATH` and `LY_PROJECTS`
    1.  Click "Generate"
    
1.  The configuration of the solution is complete. To build the Editor and AssetProcessor to binaries, run this command inside your repo:
    ```
    cmake --build <your build path> --target AutomatedTesting.GameLauncher AssetProcessor Editor --config profile -- /m
    ```
   
1.  This will compile after some time and binaries will be available in the build path you've specified

### Setting up new projects
1.  While still within the repo folder, register the engine with this command:
    ```
    scripts\o3de.bat register --this-engine
    ```
1. Setup new projects using the `o3de create-project` command.
    ```
    <Repo path>\scripts\o3de.bat create-project --project-path <your new project path>
    ```
1. Register the engine to the project
    ```
    <Repo path>\scripts\o3de.bat register --project-path <New project path>
    ```
1.  Once you're ready to build the project, run the same set of commands to configure and build:
    ```
    cmake -B <your project build path> -S <your new project source path> -G "Visual Studio 16" -DLY_3RDPARTY_PATH=<3rdParty cache path>

    cmake --build <your project build path> --target <New Project Name>.GameLauncher --config profile -- /m
    ```
  
For a tutorial on project configuration, see [Creating Projects Using the Command Line](https://docs.o3de.org/docs/welcome-guide/get-started/project-config/creating-projects-using-cli) in the documentation.

## License

For terms please see the LICENSE*.TXT file at the root of this distribution.
