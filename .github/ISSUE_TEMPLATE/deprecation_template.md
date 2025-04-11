---
name: Deprecation Notice
about: A notice for one or more feature or API deprecations
title: 'Deprecation Notice'
labels: 'needs-triage,needs-sig,kind/deprecation'

---

## Deprecated APIs
List the fully qualified APIs/types. For example, `AZ::Quaternion::SetFromEulerRadians(const Vector3&)`

## Alternatives APIs
List new or updated APIs to replace deprecated API. 
For example, use 
`Quaternion AZ::Quaternion::CreateFromEulerRadiansXYZ(const Vector3&)` 
to repleace 
`void AZ::Quaternion::SetFromEulerRadians(const Vector3&)` and `Quaternion AZ::Quaternion::ConvertEulerRadiansToQuaternion(const Vector3&)`

## Last Release
Describe the last release version before this deprecation. For example: `stabilization/2210`

## Additional Context
Add some information about the reasons for the deprecations
