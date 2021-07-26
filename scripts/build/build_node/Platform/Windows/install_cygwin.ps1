<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

# Grab credentials from parameter store (assumes AWS cli is installed and correct IAM policy is setup)
$az = curl http://169.254.169.254/latest/meta-data/placement/availability-zone -UseBasicParsing
$region = $az.Content -replace ".$"

$username = aws ssm get-parameters --names "shared.builderuser" --region $region --with-decryption | ConvertFrom-Json
$username = $username.Parameters.Value.ToString()
$password = aws ssm get-parameters --names "shared.builderpass" --region $region --with-decryption | ConvertFrom-Json
$password = ConvertTo-SecureString $password.Parameters.Value.ToString() -AsPlainText -Force
$credential = New-Object System.Management.Automation.PSCredential -ArgumentList $username, $password

$cygwin_packages = "openssh,vim,curl,tar,wget,zip,unzip,diffutils,bzr,nc,procps,ncdu"

# Download cygwin
New-Item C:\tools\cygwin -Force
wget https://cygwin.com/setup-x86_64.exe -o C:\tools\cygwin\setup-x86_64.exe

# Install cygwin (this is a specific version not subject to LGPLv3)
# complete package list: https://cygwin.com/packages/package_list.html
Write-Host "  * Starting Cygwin install"
Start-Process "C:\tools\cygwin\setup-x86_64.exe" -ArgumentList ("--quiet-mode " +
"--wait --root C:\cygwin --site http://cygwin.osuosl.org " +
"--packages $cygwin_packages") -wait `
-NoNewWindow -PassThru -RedirectStandardOutput "C:\cygwin_install.log" `
-RedirectStandardError "C:\cygwin_install.err"

# Open up firewall for ssh daemon
Write-Host "  * Opening port 22 on firewall"
New-NetFirewallRule -DisplayName "Allow SSH inbound" -Direction Inbound -LocalPort 22 -Protocol TCP -Action Allow

# Workaround for https://www.cygwin.com/ml/cygwin/2015-10/msg00036.html
# see:
#   1) https://www.cygwin.com/ml/cygwin/2015-10/msg00038.html
#   2) https://goo.gl/EWzeVV
$env:LOGONSERVER = "\\" + $env:COMPUTERNAME

# Configure sshd
Write-Host "  * Configuring SSHD"
Start-Process "C:\cygwin\bin\bash.exe" -ArgumentList "--login
-c `"ssh-host-config -y -c 'ntsec mintty' -u '$($username)' -w '$($Credential.GetNetworkCredential().Password)'`"" `
-wait -NoNewWindow -PassThru `
-RedirectStandardOutput "C:\logs\cygrunsrv.log" -RedirectStandardError "C:\logs\cygrunsrv.err"
Start-Process "C:\cygwin\bin\bash.exe" -ArgumentList "--login -c 'echo ""KexAlgorithms curve25519-sha256@libssh.org,ecdh-sha2-nistp256,ecdh-sha2-nistp384,ecdh-sha2-nistp521,diffie-hellman-group-exchange-sha256,diffie-hellman-group14-sha1,diffie-hellman-group-exchange-sha1,diffie-hellman-group1-sha1"" >> /etc/sshd_config'"

# Copy bash script to add special permissions to builder account
aws s3 cp s3://ly-jenkins-node-config/windows/setup.sh c:\cygwin\home\$username\

# Run bash setup script
echo "  * Configuring Bash"
Start-Process "C:\cygwin\bin\bash.exe" -ArgumentList "--login -c 'chmod a+x ~/setup.sh; ~/setup.sh $username'" `
-wait -NoNewWindow -PassThru `
-RedirectStandardOutput "C:\logs\Administrator_cygwin_setup.log" `
-RedirectStandardError "C:\logs\Administrator_cygwin_setup.err"

# Start sshd
echo "  * Starting SSHD"
Start-Process "net" -ArgumentList "start cygsshd" `
-wait -NoNewWindow -PassThru `
-RedirectStandardOutput "C:\logs\net_start_sshd.log" -RedirectStandardError "C:\logs\net_start_sshd.err"

# Add SSH key
echo "  * Getting SSH key"
$sshkey = aws ssm get-parameters --names "shared.buildersshkey" --region $region --with-decryption | ConvertFrom-Json
New-Item "C:\cygwin\home\$username\.ssh" -Force 
Add-Content "C:\cygwin\home\$username\.ssh\authorized_keys" "$($sshkey.Parameters.Value)"
Start-Process "C:\cygwin\bin\bash.exe" -ArgumentList "--login -c 'chown -R $username /home/$username; chmod 600 /home/$username/.ssh/authorized_keys; sed -i 's/\r$//' /home/$username/.ssh/authorized_keys'" `

# Clean up secure variables
Remove-Variable password
Remove-Variable credential
Remove-Variable sshkey