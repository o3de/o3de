#include <AzTest/AzTest.h>
#include <MotionMatchingSystemComponent.h>
#include <Tests/SystemComponentFixture.h>

namespace EMotionFX::MotionMatching
{
	using Fixture = ComponentFixture<
		AZ::MemoryComponent,
		AZ::AssetManagerComponent,
		AZ::JobManagerComponent,
		AZ::StreamerComponent,
		EMotionFX::Integration::SystemComponent,
		MotionMatchingSystemComponent
	>;
}
