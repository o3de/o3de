# Amazon Project Spectra Private Preview

Copyright (c) 2016-2021 Amazon Technologies, Inc., its affiliates or licensors.  All Rights Reserved.  




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
> git clone https://github.com/aws/o3de.git
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


## License

For complete copyright and license terms please see the LICENSE.txt file at the root of this distribution (the "License").


