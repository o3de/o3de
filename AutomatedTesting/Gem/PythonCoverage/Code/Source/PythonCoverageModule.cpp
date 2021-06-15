#include <PythonCoverageModule.h>

#pragma optimize("", off)

namespace PythonCoverage
{
    PythonCoverageModule::PythonCoverageModule()
        : AZ::Module()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(
            m_descriptors.end(),
            { PythonCoverageSystemComponent::CreateDescriptor() });
    }

    AZ::ComponentTypeList PythonCoverageModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<PythonCoverageSystemComponent>(),
        };
    }
}// namespace PythonCoverage

#if !defined(PYTHON_COVERAGE_EDITOR)
AZ_DECLARE_MODULE_CLASS(Gem_PythonCoverage, PythonCoverage::PythonCoverageModule)
#endif // !defined(PYTHON_COVERAGE_EDITOR)
