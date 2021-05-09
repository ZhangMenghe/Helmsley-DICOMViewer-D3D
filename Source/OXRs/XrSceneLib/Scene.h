#pragma once
#include <mutex>
#include <winrt/Windows.Perception.Spatial.h>

#include <Common/DeviceResources.h>
#include "FrameTime.h"
#include "XrContext.h"
namespace xr {

    struct Scene {
        virtual ~Scene() = default;
        explicit Scene(const std::shared_ptr<xr::XrContext>& context);

        Scene() = delete;
        Scene(Scene&&) = delete;
        Scene(const Scene&) = delete;

        virtual void Update(const FrameTime& frameTime) = 0;
        virtual void BeforeRender(const FrameTime& frameTime) = 0;
        virtual void Render(const xr::FrameTime& frameTime, uint32_t view_id) = 0;
        virtual void onSingle3DTouchDown(float x, float y, float z, int side) = 0;
        virtual void on3DTouchMove(float x, float y, float z, glm::mat4 rot, int side) = 0;
        virtual void on3DTouchReleased(int side) = 0;

        virtual void SetupDeviceResource(const std::shared_ptr<DX::DeviceResources>& deviceResources){}
        virtual void SetupReferenceFrame(winrt::Windows::Perception::Spatial::SpatialCoordinateSystem referenceFrame){}
        virtual void saveCurrentTargetViews(winrt::com_ptr <ID3D11RenderTargetView> render_target, 
            winrt::com_ptr<ID3D11DepthStencilView> depth_target, float depth_value){}

        // Active is true when the scene participates update and render loop.
        bool IsActive() const {
            return m_isActive;
        }
        void SetActive(bool active) {
            if (m_isActive != active) {
                m_isActive = active;
                //OnActiveChanged();
            }
        }
        void ToggleActive() {
            SetActive(!IsActive());
        }

        void NotifyEvent(const XrEventDataBuffer& eventData) {
            //OnEvent(eventData);
        }

    protected:
        std::shared_ptr<xr::XrContext> m_context;
        //winrt::com_ptr<ID3D11DepthStencilView> m_depthStencilView = nullptr;
        //virtual void OnUpdate(const xr::FrameTime& frameTime [[maybe_unused]]) {
        //}
        //virtual void OnBeforeRender(const xr::FrameTime& frameTime [[maybe_unused]]) const {
        //}
        //virtual void OnEvent(const XrEventDataBuffer& eventData [[maybe_unused]]) {
        //}
        //virtual void OnActiveChanged() {
        //}

    private:
        std::atomic<bool> m_isActive{true};
        //mutable std::mutex m_uninitializedMutex;
    };

} // namespace engine
