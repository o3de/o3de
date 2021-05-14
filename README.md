# Project Spectra Private Preview

## Confidentiality; Pre-Release Access  

Welcome to the Project Spectra Private Preview.  This is a confidential pre-release project; your use is subject to the nondisclosure agreement between you (or your organization) and Amazon.  Do not disclose the existence of this project, your participation in it, or any of the  materials provided, to any unauthorized third party.  To request access for a third party, please contact [Royal O'Brien, obriroya@amazon.com](mailto:obriroya@amazon.com).

## Full instructions can be found here: 
## https://docs.o3de.org/docs/welcome-guide/setup/setup-from-github/ (Note requires registration)

## Download and Install

This repository uses Git LFS for storing large binary files.  You will need to create a Github personal access token to authenticate with the LFS service.


### Create a Git Personal Access Token

You will need your personal access token credentials to authenticate when you clone the repository.

[Create a personal access token with the 'repo' scope.](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token)


### (Recommended) Verify you have a credential manager installed to store your credentials 

Recent versions of Git install a credential manager to store your credentials so you don't have to put in the credentials for every request.  
It is highly recommended you check that you have a [credential manager installed and configured](https://github.com/microsoft/Git-Credential-Manager-Core)


### Clone the repository 

```shell
> git clone https://github.com/aws-lumberyard/o3de.git
Cloning into 'o3de'...

# initial prompt for credentials to download the repository code
# enter your Github username and personal access token

remote: Counting objects: 29619, done.
Receiving objects: 100% (29619/29619), 40.50 MiB | 881.00 KiB/s, done.
Resolving deltas: 100% (8829/8829), done.
Updating files: 100% (27037/27037), done.

# second prompt for credentials when downloading LFS files
# enter your Github username and personal access token

Filtering content: 100% (3853/3853), 621.43 MiB | 881.00 KiB/s, done.

```

If you have the Git credential manager core installed, you should not be prompted for your credentials anymore.

## Building the Engine
### Build Requirements and redistributables
#### Windows
*   Visual Studio 2019 16.9.2 minimum (All versions supported, including Community): [https://visualstudio.microsoft.com/downloads/](https://visualstudio.microsoft.com/downloads/)
    *   Install the following workloads:
        *   Game Development with C++
        *   MSVC v142 - VS 2019 C++ x64/x86
*   Visual C++ redistributable: [https://visualstudio.microsoft.com/downloads/#other-family](https://visualstudio.microsoft.com/downloads/#other-family)
*   CMake 3.19.1 minimum: [https://cmake.org/files/LatestRelease/cmake-3.19.1-win64-x64.msi](https://cmake.org/files/LatestRelease/cmake-3.19.1-win64-x64.msi)

#### Optional

*   WWise - 2019.2.8.7432: [https://www.audiokinetic.com/download/](https://www.audiokinetic.com/download/)
    *   Note: This requires registeration and installation of a client to download
### Quick Start Build Steps

1.  Create a writable folder to cache 3rd Party dependancies. You can also use this to store other redistributable SDKs.

2.  Install the following redistributables to the following:
    - Visual Studio and VC++ redistributable can be installed to any location
    - CMake can be installed to any location, as long as it's available in the system path, otherwise it can be installed to: `<3rdParty Path>\CMake\3.19.1`
    - WWise should be installed to: `<3rdParty Path>\Wwise\2019.2.8.7432`
    
3.  Add the following environment variables through the command line
    ```
    set 3RDPARTY_PATH=<Location of the 3rdParty cache>
    ```
    
4.  Navigate into the repo folder, then register the engine with this command:
    `scripts\o3de.bat register --this-engine`

5.  Configure the source into a solution using this command line, replacing <your build location> to a path you've created
    ```
    cmake -B <your build location> -S <source-dir> -G "Visual Studio 16 2019" -DLY_3RDPARTY_PATH=%3RDPARTY_PATH% -DLY_UNITY_BUILD=ON -DLY_PROJECTS=AutomatedTesting 
    ```

6.  Alternatively, you can do this through the CMake GUI:
    
    1.  Start `cmake-gui.exe`
    2.  Select the local path of the repo under "Where is the source code"
    3.  Select a path where to build binaries under "Where to build the binaries"
    4.  Click "Configure"
    5.  Wait for the key values to populate. Fill in the fields that are relevant, including `LY_3RDPARTY_PATH`, `LY_PACKAGE_SERVER_URLS`, and `LY_PROJECTS`
    6.  Click "Generate"
    
7.  The configuration of the solution is complete. To build the Editor and AssetProcessor to binaries, run this command inside your repo:
    ```
    cmake --build <your build location> --target AutomatedTesting.GameLauncher AssetProcessor Editor --config profile -- /m
    ```
   
8.  This will compile after some time and binaries will be available in the build path you've specified

### Setting up new projects    
1. Setup new projects using this command
    ```
    <Repo path>\scripts\o3de.bat create-project --project-path <New project path>
    ```
2. Register the engine to the project
    ```
    <Repo path>\scripts\o3de.bat register --project-path <New project path>
    ```
4.  Once you're ready to build the project, run the same set of commands to configure and build:
    ```
    cmake -B <your build location in the project> -S <New project path> -G "Visual Studio 16" -DLY_3RDPARTY_PATH=%LY_3RDPARTY_PATH%

    cmake --build <your build location in the project> --target <New Project Name> --config profile -- /m
    ```

## License

For terms please see the LICENSE*.TXT file at the root of this distribution.
