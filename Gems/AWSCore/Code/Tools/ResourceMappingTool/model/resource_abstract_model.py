"""
Copyright (c) Contributors to the Open 3D Engine Project

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from abc import ABCMeta, abstractmethod

from model.basic_resource_attributes import BasicResourceAttributes


class ResourceAbstractModel(metaclass=ABCMeta):
    """ResourceAbstractModel declares the shared interfaces among all custom resource model"""
    @abstractmethod
    def load_resource(self, resource: BasicResourceAttributes) -> None:
        """Load individual resource which will not invoke data changed signal"""
        raise NotImplementedError

    @abstractmethod
    def reset(self) -> None:
        """Reset model resources"""
        raise NotImplementedError
