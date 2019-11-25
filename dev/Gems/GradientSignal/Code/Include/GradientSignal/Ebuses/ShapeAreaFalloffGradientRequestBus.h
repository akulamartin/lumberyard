/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <GradientSignal/GradientSampler.h>

namespace GradientSignal
{
    enum class FalloffType : AZ::u8
    {
        Inner = 0,
        Outer,
        InnerOuter,
    };

    class ShapeAreaFalloffGradientRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual AZ::EntityId GetShapeEntityId() const = 0;
        virtual void SetShapeEntityId(AZ::EntityId entityId) = 0;

        virtual float GetFalloffWidth() const = 0;
        virtual void SetFalloffWidth(float falloffWidth) = 0;

        virtual FalloffType GetFalloffType() const = 0;
        virtual void SetFalloffType(FalloffType type) = 0;
    };

    using ShapeAreaFalloffGradientRequestBus = AZ::EBus<ShapeAreaFalloffGradientRequests>;
}