## Updates to this readme
July 06, 2021
- Switch licenses to APACHE-2.0 OR MIT

May 14, 2021 
- Removed instructions for the 3rdParty zip file and downloader URL. This is no longer a requirement. 
- Updated instructions for dependencies
- Links to full documentation

April 7-13, 2021
- Updates to the 3rdParty zip file

March 25, 2021
- Initial commit for instructions

## Download and Install

This repository uses Git LFS for storing large binary files.  You will need to create a Github personal access token to authenticate with the LFS service.

To install Git LFS, download the binary here: https://git-lfs.github.com/.

After installation, you will need to install the necessary git hooks with this command 
```
git lfs install
```

### Create a Git Personal Access Token

You will need your personal access token credentials to authenticate when you clone the repository and when downloading objects from Git LFS

[Create a personal access token with the 'repo' scope.](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token)

During the clone operation, you will be prompted to enter a password. Your token will be used as the password. You will also be prompted a second time for Git LFS.

### (Recommended) Verify you have a credential manager installed to store your credentials 

Recent versions of Git install a credential manager to store your credentials so you don't have to put in the credentials for every request.

It is highly recommended you check that you have a [credential manager installed and configured](https://github.com/microsoft/Git-Credential-Manager-Core)

For Linux and Mac, use the following commands to store credentials 

Linux: 
```
git config --global credential.helper cache
``` 
Mac:
```
git config --global credential.helper osxkeychain
```

### Clone the repository 

```shell
> git clone https://github.com/o3de/o3de.git
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

If you have the Git credential manager core or other credential helpers installed, you should not be prompted for your credentials anymore.

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

*   WWise - 2019.2.8.7432 minimum: [https://www.audiokinetic.com/download/](https://www.audiokinetic.com/download/)
    *   Note: This requires registration and installation of a client to download
    *   You will also need to set a environment variable: `set LY_WWISE_INSTALL_PATH=<path to WWise version>`
    *   For example: `set LY_WWISE_INSTALL_PATH="C:\Program Files (x86)\Audiokinetic\Wwise 2019.2.8.7432"`

### Quick Start Build Steps

1.  Create a writable folder to cache 3rd Party dependencies. You can also use this to store other redistributable SDKs.
    
    > For the 0.5 branch - Create an empty text file named `3rdParty.txt` in this folder, to allow a legacy CMake validator to pass

1.  Install the following redistributables to the following:
    - Visual Studio and VC++ redistributable can be installed to any location
    - CMake can be installed to any location, as long as it's available in the system path
    - WWise can be installed anywhere, but you will need to set an environment variable for CMake to detect it:  `set LY_WWISE_INSTALL_PATH=<path to WWise>`
    
1.  Navigate into the repo folder, then download the python runtime with this command
    
    > For the 0.5 branch - Set this environment variable prior to the `get_python` command below:
    > ```
    > set LY_PACKAGE_SERVER_URLS=https://d2c171ws20a1rv.cloudfront.net
    > ```

    ```
    python\get_python.bat
    ```

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
1. Setup new projects using the `o3de create-project` command. In the 0.5 branch, the project directory must be a subdirectory in the repo folder.
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

    // For the 0.5 branch, you must build a new Editor for each project:
    cmake --build <your project build path> --target <New Project Name>.GameLauncher Editor --config profile -- /m
    
    // For all other branches, just build the project:
    cmake --build <your project build path> --target <New Project Name>.GameLauncher --config profile -- /m
    ```
  
For a tutorial on project configuration, see [Creating Projects Using the Command Line](https://docs.o3de.org/docs/welcome-guide/get-started/project-config/creating-projects-using-cli) in the documentation.

## License

For terms please see the LICENSE*.TXT file at the root of this distribution.
