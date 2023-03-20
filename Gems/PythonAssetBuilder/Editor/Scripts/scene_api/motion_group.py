#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
from enum import Enum
import scene_api.common_rules

class MotionGroup(scene_api.common_rules.BaseRule):
    """
    Configure animation data for exporting.

    Attributes
    ----------
    name: str
        Name for the group.
        This name will also be used as the name for the generated file.

    selectedRootBone: str
        The root bone of the animation that will be exported.

    rules: list of BaseRule
        Add or remove rules to fine-tune the export process.
        List of rules for a motion group including:
            MotionScaleRule
            CoordinateSystemRule
            MotionRangeRule
            MotionAdditiveRule
            MotionSamplingRule

    """
    def __init__(self):
        super().__init__('MotionGroup')
        self.name = ''
        self.selectedRootBone = ''
        self.rules = set()

    def add_rule(self, rule) -> bool:
        if (rule not in self.rules):
            self.rules.add(rule)
            return True
        return False

    def create_rule(self, rule) -> any:
        if (self.add_rule(rule)):
            return rule
        return None

    def remove_rule(self, type) -> None:
        self.rules.discard(rule)

    def to_dict(self) -> dict:
        out = super().to_dict()
        out['name'] = self.name
        out['selectedRootBone'] = self.selectedRootBone
        # convert the rules
        ruleList = []
        for rule in self.rules:
            ruleList.append(rule.to_dict())
        out['rules'] = ruleList
        return out

    def to_json(self) -> str:
        jsonDOM = self.to_dict()
        return json.dumps(jsonDOM, cls=RuleEncoder)


class MotionCompressionSettingsRule(scene_api.common_rules.BaseRule):
    """
    A BaseRule that ses the error tolerance settings while compressing the animation

    Attributes
    ----------
    maxTranslationError: float
        Maximum error allowed in translation.
        Min 0.0, Max 0.1

    maxRotationError: float
        Maximum error allowed in rotation.
        Min 0.0, Max 0.1

    maxScaleError: float
        Maximum error allowed in scale.
        Min 0.0, Max 0.01
    """
    def __init__(self):
        super().__init__('MotionCompressionSettingsRule')
        self.maxTranslationError = 0.0001
        self.maxRotationError = 0.0001
        self.maxScaleError = 0.0001

class MotionScaleRule(scene_api.common_rules.BaseRule):
    """
    A BaseRule that scales the spatial extent of motion

    Attributes
    ----------
    scaleFactor: float
        Scale factor; min 0.0001, max 10000.0

    """
    def __init__(self):
        super().__init__('MotionScaleRule')
        self.scaleFactor = 1.0


class MotionRangeRule(scene_api.common_rules.BaseRule):
    """
    A BaseRule that defines the range of the motion that will be exported.

    Attributes
    ----------
    startFrame: float
        The start frame of the animation that will be exported.

    endFrame: float
        The end frame of the animation that will be exported.
    """
    def __init__(self):
        super().__init__('MotionRangeRule')
        self.startFrame = 0
        self.endFrame = 0


class MotionAdditiveRule(scene_api.common_rules.BaseRule):
    """
    A BaseRule that makes the motion an additive motion.

    Attributes
    ----------
    sampleFrame: int
        The frame number that the motion will be made relative to.

    """
    def __init__(self):
        super().__init__('MotionAdditiveRule')
        self.sampleFrame = 0


class SampleRateMethod(Enum):
    """
    A collection of settings related to sampling of the motion

    Attributes
    ----------

    FromSourceScene: int, value = 0
        Use the source scene's sample rate


    Custom: int, value = 1
        Use the use a custom sample rate
    """
    FromSourceScene = 0
    Custom = 1

    def to_json_value(self):
        if(self == SampleRateMethod.FromSourceScene):
            return 0
        return 1


class MotionSamplingRule(scene_api.common_rules.BaseRule):
    """
    A collection of settings related to sampling of the motion

    Attributes
    ----------
    motionDataType: scene_api.common_rules.TypeId()
        The motion data type to use. This defines how the motion data is stored.
        This can have an effect on performance and memory usage.

    sampleRateMethod: SampleRateMethod
        Either use the sample rate from the source scene file or use a custom sample rate.
        The sample rate is automatically limited to the rate from source scene file (e.g. FBX)

    customSampleRate: float
        Overwrite the sample rate of the motion, in frames per second.
        Min: 1.0, Max 240.0

    translationQualityPercentage: float
        The percentage of quality for translation. Higher values preserve quality, but increase memory usage.
        Min: 1.0, Max 100.0

    rotationQualityPercentage: float
        The percentage of quality for rotation. Higher values preserve quality, but increase memory usage.
        Min: 1.0, Max 100.0

    scaleQualityPercentage: float
        The percentage of quality for scale. Higher values preserve quality, but increase memory usage.
        Min: 1.0, Max 100.0

    allowedSizePercentage: float
        The percentage of extra memory usage allowed compared to the smallest size.
        For example a value of 10 means we are allowed 10 percent more memory worst case, in trade for extra performance.
        Allow 15 percent larger size, in trade for performance (in Automatic mode, so when m_motionDataType is a Null typeId).
        Min: 0.0, Max 100.0

    keepDuration: bool
        When enabled this keep the duration the same as the Fbx motion duration, even if no joints are animated.
        When this option is disabled and the motion doesn't animate any joints then the resulting motion will have a duration of zero seconds.
    """
    def __init__(self):
        super().__init__('MotionSamplingRule')
        self.motionDataType = scene_api.common_rules.TypeId()
        self.sampleRateMethod = SampleRateMethod.FromSourceScene
        self.customSampleRate = 60.0
        self.translationQualityPercentage = 75.0
        self.rotationQualityPercentage = 75.0
        self.scaleQualityPercentage = 75.0
        self.allowedSizePercentage = 15.0
        self.keepDuration = True
