﻿#ifndef SCENARIO_H
#define SCENARIO_H

#include "pch.h"
#include <Common/DeviceResources.h>
#include <Common/StepTimer.h>
#include <winrt/Windows.UI.Core.h>

class Scenario
{
public:
    Scenario(std::shared_ptr<DX::DeviceResources> const& deviceResources) :
        m_deviceResources(deviceResources)
    {
    }

    ~Scenario()
    {
    }

    virtual void IntializeSensors() = 0;
    virtual void IntializeScene() = 0;
    //virtual void PositionHologram(winrt::Windows::UI::Input::Spatial::SpatialPointerPose const& pointerPose, const DX::StepTimer& timer) = 0;
    //virtual void PositionHologramNoSmoothing(winrt::Windows::UI::Input::Spatial::SpatialPointerPose const& pointerPose) = 0;
    virtual void Update(DX::StepTimer& timer) = 0;
    //virtual winrt::Windows::Foundation::Numerics::float3 const& GetPosition() = 0;
    virtual void Render() = 0;

    virtual void Update()
    {
    }

    //void SetStationaryFrameOfReference(winrt::Windows::Perception::Spatial::SpatialStationaryFrameOfReference const& stationaryReferenceFrame)
    //{
    //    m_stationaryReferenceFrame = stationaryReferenceFrame;
    //}

    virtual void OnDeviceLost() = 0;
    virtual void OnDeviceRestored() = 0;

protected:
    std::shared_ptr<DX::DeviceResources> m_deviceResources;
    //winrt::Windows::Perception::Spatial::SpatialStationaryFrameOfReference m_stationaryReferenceFrame = nullptr;

};
#endif // !SCENARIO_H
