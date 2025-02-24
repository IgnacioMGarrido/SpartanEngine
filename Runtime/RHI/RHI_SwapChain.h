/*
Copyright(c) 2016-2022 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= INCLUDES =====================
#include <memory>
#include <vector>
#include <array>
#include "RHI_Definition.h"
#include "../Core/SpartanObject.h"
//================================

namespace Spartan
{
    namespace Math { class Vector4; }

    class SPARTAN_CLASS RHI_SwapChain : public SpartanObject
    {
    public:
        RHI_SwapChain(
            void* window_handle,
            const std::shared_ptr<RHI_Device>& rhi_device,
            uint32_t width,
            uint32_t height,
            RHI_Format format ,
            uint32_t buffer_count,
            uint32_t flags,
            const char* name
        );
        ~RHI_SwapChain();

        bool Resize(uint32_t width, uint32_t height, const bool force = false);
        void Present();

        // Layout
        RHI_Image_Layout GetLayout() const { return m_layouts[m_index_image_acquired]; }
        void SetLayout(const RHI_Image_Layout& layout, RHI_CommandList* cmd_list);

        // Misc
        uint32_t GetWidth()       const { return m_width; }
        uint32_t GetHeight()      const { return m_height; }
        uint32_t GetBufferCount() const { return m_buffer_count; }
        uint32_t GetFlags()       const { return m_flags; }
        uint32_t GetImageIndex()  const { return m_index_image_acquired; }
        bool PresentEnabled()     const { return m_present_enabled; }

        // GPU Resources
        void* Get_Resource()                   const { return m_backbuffer_resource[0]; }
        void* Get_Resource_View()              const { return m_backbuffer_resource_view[m_index_image_acquired]; }
        void* Get_Resource_View_RenderTarget() const { return m_resource_view; }

    private:
        void AcquireNextImage();

        // Properties
        bool m_windowed                           = false;
        uint32_t m_buffer_count                   = 0;
        uint32_t m_width                          = 0;
        uint32_t m_height                         = 0;
        uint32_t m_flags                          = 0;
        RHI_Format m_format                       = RHI_Format_R8G8B8A8_Unorm;
        std::array<RHI_Image_Layout, 4> m_layouts = { RHI_Image_Layout::Undefined, RHI_Image_Layout::Undefined, RHI_Image_Layout::Undefined, RHI_Image_Layout::Undefined };

        // API
        void* m_resource           = nullptr;
        void* m_resource_view      = nullptr;
        void* m_surface            = nullptr;
        void* m_window_handle      = nullptr;
        bool m_present_enabled     = true;
        uint32_t m_semaphore_index = std::numeric_limits<uint32_t>::max();
        RHI_Device* m_rhi_device   = nullptr;

        // Sync objects
        std::array<std::shared_ptr<RHI_Semaphore>, 3> m_semaphore_image_acquired;
        uint32_t m_index_image_acquired = std::numeric_limits<uint32_t>::max();

        // Resource
        std::array<void*, 3> m_backbuffer_resource;
        std::array<void*, 3> m_backbuffer_resource_view;
    };
}
