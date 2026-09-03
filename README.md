# O3DE (Open 3D Engine)

O3DE (Open 3D Engine) is an open-source, real-time, multi-platform 3D engine that enables developers and content creators to build AAA games, cinema-quality 3D worlds, and high-fidelity simulations without any fees or commercial obligations.

## Documentation

For the full O3DE documentation, visit [https://o3de.org/docs/](https://o3de.org/docs/).

## Contribute

For information about contributing to Open 3D Engine, visit [https://o3de.org/docs/contributing/](https://o3de.org/docs/contributing/).

## Roadmap

For information about upcoming work and features, please visit [https://o3de.org/roadmap](https://o3de.org/roadmap). Progress against the roadmap is tracked [here](https://github.com/orgs/o3de/projects/56/views/2).

## Prebuilt Installer

If you don't want to build the engine from source, prebuilt installers are available:

*   [Windows](https://o3debinaries.org/download/windows.html)
*   [Linux](https://o3debinaries.org/download/linux.html)

## Download and Install

This repository uses Git LFS for storing large binary files.  

Verify you have Git LFS installed by running the following command to print the version number.
```
git lfs --version 
```

If Git LFS is not installed, download and run the installer from: [https://git-lfs.github.com/](https://git-lfs.github.com/).

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

For the latest details and system requirements, refer to [System Requirements](https://o3de.org/docs/welcome-guide/requirements/) in the documentation.

#### Windows

*   Visual Studio 2022 latest (All editions supported, including Community): [https://visualstudio.microsoft.com/downloads/](https://visualstudio.microsoft.com/downloads/)
    *   Install the following workloads:
        *   Game Development with C++
        *   MSVC v142 - VS 2019 C++ x64/x86 minimum
        *   C++ 2019 redistributable update (C++ 2022 redistributable recommended)
*   CMake 3.30 minimum: [https://cmake.org/download/#latest](https://cmake.org/download/#latest) (Release Candidate versions are not supported)
    *   CMake 3.30 is currently a soft requirement and will become a hard requirement after release 26.05.
    *   CMake 4.2.3 is bundled with the [prebuilt installer](https://o3debinaries.org/download/windows.html).

#### Linux

*   Clang 14 minimum
*   CMake 3.30 minimum: [https://cmake.org/download/#latest](https://cmake.org/download/#latest) (Release Candidate versions are not supported)
    *   CMake 3.30 is currently a soft requirement and will become a hard requirement after release 26.05.
    *   CMake 4.2.3 is bundled with the [prebuilt installer](https://o3debinaries.org/download/linux.html).

#### Optional

*   Wwise audio SDK
    *   For the latest version requirements and setup instructions, refer to the [Wwise Audio Engine Gem](https://o3de.org/docs/user-guide/gems/reference/audio/wwise/audio-engine-wwise/) reference in the documentation.

### Quick start: Building the engine

For a complete setup guide, refer to [Setting up O3DE from GitHub](https://o3de.org/docs/welcome-guide/setup/setup-from-github/) in the documentation.

#### Windows

1.  Bootstrap the O3DE Python runtime:
    ```
    python\get_python.bat
    ```

1.  Register the engine:
    ```
    scripts\o3de.bat register --this-engine
    ```

1.  Configure a solution, replacing `<build path>` and `<3rdParty package path>` with your chosen locations:
    ```
    cmake -B <build path> -S . -G "Visual Studio 16 2019" -DLY_3RDPARTY_PATH=<3rdParty package path>
    ```

    Example:
    ```
    cmake -B C:\o3de\build\windows -S C:\o3de -G "Visual Studio 16 2019" -DLY_3RDPARTY_PATH=C:\o3de-packages
    ```

    > Note: Do not use trailing slashes for the `<3rdParty package path>`.

1.  Build the Editor:
    ```
    cmake --build <build path> --target Editor --config profile -- /m
    ```

#### Linux

1.  Bootstrap the O3DE Python runtime:
    ```
    python/get_python.sh
    ```

1.  Register the engine:
    ```
    scripts/o3de.sh register --this-engine
    ```

1.  Configure a solution, replacing `<build path>` and `<3rdParty package path>` with your chosen locations:
    ```
    cmake -B <build path> -S . -G "Ninja Multi-Config" -DLY_3RDPARTY_PATH=<3rdParty package path> -DCMAKE_C_COMPILER=clang-14 -DCMAKE_CXX_COMPILER=clang++-14
    ```

    Example:
    ```
    cmake -B ~/o3de/build/linux -S ~/o3de -G "Ninja Multi-Config" -DLY_3RDPARTY_PATH=~/o3de-packages -DCMAKE_C_COMPILER=clang-14 -DCMAKE_CXX_COMPILER=clang++-14
    ```

    > Note: Do not use trailing slashes for the `<3rdParty package path>`.

1.  Build the Editor:
    ```
    cmake --build <build path> --target Editor --config profile
    ```

### Quick start: Building a project

For a complete guide, refer to [Creating Projects Using the Command Line Interface](https://o3de.org/docs/welcome-guide/create/creating-projects-using-cli/) in the documentation.

#### Windows

1.  Create a new project from the O3DE repo folder:
    ```
    scripts\o3de.bat create-project --project-path <your new project path>
    ```

1.  Register the project with the engine:
    ```
    scripts\o3de.bat register --project-path <your new project path>
    ```

1.  Configure a solution for the project:
    ```
    cmake -B <project build path> -S <your new project path> -G "Visual Studio 16 2019" -DLY_3RDPARTY_PATH=<3rdParty package path>
    ```

    Example:
    ```
    cmake -B C:\my-project\build\windows -S C:\my-project -G "Visual Studio 16 2019" -DLY_3RDPARTY_PATH=C:\o3de-packages
    ```

    > Note: Do not use trailing slashes for the `<3rdParty package path>`.

1.  Build the project, Asset Processor, and Editor:
    ```
    cmake --build <project build path> --target <ProjectName>.GameLauncher Editor --config profile -- /m
    ```

    > Note: The build target name matches the project directory name. Binaries will be available under `bin/profile`.

#### Linux

1.  Create a new project from the O3DE repo folder:
    ```
    scripts/o3de.sh create-project --project-path <your new project path>
    ```

1.  Register the project with the engine:
    ```
    scripts/o3de.sh register --project-path <your new project path>
    ```

1.  Configure a solution for the project:
    ```
    cmake -B <project build path> -S <your new project path> -G "Ninja Multi-Config" -DLY_3RDPARTY_PATH=<3rdParty package path> -DCMAKE_C_COMPILER=clang-14 -DCMAKE_CXX_COMPILER=clang++-14
    ```

    Example:
    ```
    cmake -B ~/my-project/build/linux -S ~/my-project -G "Ninja Multi-Config" -DLY_3RDPARTY_PATH=~/o3de-packages -DCMAKE_C_COMPILER=clang-14 -DCMAKE_CXX_COMPILER=clang++-14
    ```

1.  Build the project, Asset Processor, and Editor:
    ```
    cmake --build <project build path> --target <ProjectName>.GameLauncher Editor --config profile
    ```

    > Note: The build target name matches the project directory name. Binaries will be available under `bin/profile`.

### Script-only projects

Script-only projects contain no C++ code and use scripting languages such as Lua. They do not require a compiler, but **must** be built against a prebuilt O3DE installation — building a script-only project against a source engine is not supported.

On Windows, script-only projects also require the `Ninja Multi-Config` generator; the Visual Studio generator does not support script-only mode.

For setup instructions, refer to [Setting up O3DE from GitHub](https://o3de.org/docs/welcome-guide/setup/setup-from-github/) in the documentation.

## Code Contributors

This project exists thanks to all the people who contribute. [[Contribute](CONTRIBUTING.md)].

<a href="https://github.com/o3de/o3de/graphs/contributors"><img src="https://contrib.rocks/image?repo=o3de/o3de&max=200&columns=24" width=850px /></a>

## License

For terms please see the LICENSE*.TXT files at the root of this distribution.
