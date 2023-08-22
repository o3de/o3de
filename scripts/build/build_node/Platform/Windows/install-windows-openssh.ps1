<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

if (-Not (New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "Must run as administrator!"
    Sleep 5
    Exit
  }
Write-Host "Setting up OpenSSH server/client"
Get-WindowsCapability -Online | Where-Object Name -like 'OpenSSH*' | Write-Host

Add-WindowsCapability -Online -Name OpenSSH.Client~~~~0.0.1.0
Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0

Write-Host "Starting OpenSSH service"
Start-Service sshd
Set-Service -Name sshd -StartupType 'Automatic'

Write-Host "Opening firewall ruls for OpenSSH"
if (!(Get-NetFirewallRule -Name "OpenSSH-Server" -ErrorAction SilentlyContinue | Select-Object Name, Enabled)) {
    Write-Output "Firewall Rule 'OpenSSH-Server' does not exist, creating it..."
    New-NetFirewallRule -Name 'OpenSSH-Server' -DisplayName 'OpenSSH Server (sshd)' -Enabled True -Direction Inbound -Protocol TCP -Action Allow -LocalPort 22
} else {
    Write-Output "Firewall rule 'OpenSSH-Server' has been created and exists."
}

Write-Host "Initializing administrators_authorized_keys"
New-Item C:\ProgramData\ssh\administrators_authorized_keys

Write-Host "Setting permissions"
$acl = Get-Acl C:\ProgramData\ssh\administrators_authorized_keys
$acl.SetAccessRuleProtection($true, $false)
$adminRule = New-Object system.security.accesscontrol.filesystemaccessrule("Administrators", "FullControl", "Allow")
$systemRule = New-Object system.security.accesscontrol.filesystemaccessrule("SYSTEM", "FullControl", "Allow")
$acl.SetAccessRule($adminRule)
$acl.SetAccessRule($systemRule)
$acl | Set-Acl
