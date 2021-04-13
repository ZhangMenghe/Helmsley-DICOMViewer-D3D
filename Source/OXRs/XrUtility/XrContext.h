// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//#include <pbr/PbrResources.h>
//#include <XrUtility/XrString.h>
#include "XrInstanceContext.h"
#include "XrExtensionContext.h"
#include "XrSystemContext.h"
#include "XrSessionContext.h"

namespace xr {

    // Session-related resources shared across multiple Scenes.
    struct XrContext final {
        XrContext(xr::InstanceContext instance,
                xr::ExtensionContext extensions,
                xr::SystemContext system,
                xr::SessionContext session
                //XrSpace appSpace,
                //Pbr::Resources pbrResources,
                //winrt::com_ptr<ID3D11Device> device,
                //winrt::com_ptr<ID3D11DeviceContext> deviceContext
        )
            : Instance(std::move(instance))
            , Extensions(std::move(extensions))
            , System(std::move(system))
            , Session(std::move(session))
            //, AppSpace(appSpace)
            //, PbrResources(std::move(pbrResources))
            //, Device(std::move(device))
            //, DeviceContext(std::move(deviceContext))
        {
        }

        const xr::InstanceContext Instance;
        const xr::ExtensionContext Extensions;
        const xr::SystemContext System;
        const xr::SessionContext Session;

        //const XrSpace AppSpace;

        //const winrt::com_ptr<ID3D11DeviceContext> DeviceContext;
        //const winrt::com_ptr<ID3D11Device> Device;
        //Pbr::Resources PbrResources;
    };

} // namespace engine
