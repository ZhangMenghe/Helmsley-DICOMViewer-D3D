//*********************************************************
//    Copyright (c) Microsoft. All rights reserved.
//
//    Apache 2.0 License
//
//    You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
//*********************************************************
#pragma once

#include <Utils/XrMath.h>
#include <pbr/PbrResources.h>
#include "ObjectMotion.h"
#include <Utils/XrExtensionContext.h>

struct Context {
  Pbr::Resources* PbrResources;
  ID3D11DeviceContext* DeviceContext;
  xr::ExtensionContext* Extensions;
};

struct FrameTime {
  using clock = std::chrono::high_resolution_clock;

  uint64_t FrameIndex = 0;
  const clock::time_point StartTime = clock::now();
  clock::time_point Now = StartTime;
  clock::duration Elapsed = {};
  clock::duration TotalElapsed = {};
  float ElapsedSeconds = {};
  float TotalElapsedSeconds = {};

  XrTime PredictedDisplayTime = {};
  XrDuration PredictedDisplayPeriod = {};
  bool ShouldRender = {};

  void Update(const XrFrameState& frameState) {
    FrameIndex++;
    PredictedDisplayTime = frameState.predictedDisplayTime;
    PredictedDisplayPeriod = frameState.predictedDisplayPeriod;
    ShouldRender = frameState.shouldRender;

    const auto now = FrameTime::clock::now();
    Elapsed = now - Now;
    Now = now;
    TotalElapsed = now - StartTime;

    ElapsedSeconds = std::chrono::duration_cast<std::chrono::duration<float>>(Elapsed).count();
    TotalElapsedSeconds = std::chrono::duration_cast<std::chrono::duration<float>>(TotalElapsed).count();
  }
};

    enum class ObjectState { InitializePending, Initialized, RemovePending };

    class Object {
    public:
        virtual ~Object() = default;
        ObjectState State;
        Motion Motion;

    public:
        void SetParent(std::shared_ptr<Object> parent) {
            m_parent = std::move(parent);
        }

        void SetVisible(bool visible) {
            m_isVisible = visible;
        }
        bool IsVisible() const {
            return State == ObjectState::Initialized && m_isVisible && (m_parent ? m_parent->IsVisible() : true);
        }

        void SetOnlyVisibleForViewIndex(uint32_t viewIndex);
        bool IsVisibleForViewIndex(uint32_t viewIndex) const;

        const XrPosef& Pose() const {
            return m_pose;
        }
        XrPosef& Pose() {
            m_localTransformDirty = true;
            return m_pose;
        }

        const XrVector3f& Scale() const {
            return m_scale;
        }
        XrVector3f& Scale() {
            m_localTransformDirty = true;
            return m_scale;
        }

        DirectX::XMMATRIX LocalTransform() const;
        DirectX::XMMATRIX WorldTransform() const;

        virtual void Update(Context& context, const FrameTime& frameTime);
        virtual void Render(Context& context) const;

    private:
        bool m_isVisible{true};

        XrPosef m_pose = xr::math::Pose::Identity();
        XrVector3f m_scale = {1, 1, 1};

        std::shared_ptr<Object> m_parent;

        // Scene Object is visible to all views by default
        struct ViewIndexMask {
            // bit location = view index
            uint32_t m_mask{static_cast<uint32_t>(-1)};
            static constexpr uint32_t MaxViewCount = sizeof(m_mask) * 8;
        } m_visibleViewIndexMask{};

        // Local transform is relative to parent
        // Only recompute when transform is changed.
        mutable DirectX::XMFLOAT4X4 m_localTransform;
        mutable bool m_localTransformDirty{true};
    };

    inline std::shared_ptr<Object> CreateObject() {
        return std::make_shared<Object>();
    }
