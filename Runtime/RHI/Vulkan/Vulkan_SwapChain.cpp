/*
Copyright(c) 2016-2021 Panos Karabelas

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

//= INCLUDES ========================
#include "Spartan.h"
#include "../RHI_Implementation.h"
#include "../RHI_SwapChain.h"
#include "../RHI_Device.h"
#include "../RHI_CommandList.h"
#include "../RHI_Pipeline.h"
#include "../RHI_Semaphore.h"
#include "../RHI_CommandPool.h"
#include "../../Profiling/Profiler.h"
#include "../../Rendering/Renderer.h"
//===================================

//= NAMESPACES ===============
using namespace std;
using namespace Spartan::Math;
//============================

namespace Spartan
{
    static void swapchain_create
    (
        RHI_Device* rhi_device,
        uint32_t* width,
        uint32_t* height,
        uint32_t buffer_count,
        RHI_Format format,
        array<RHI_Image_Layout, 4> layouts,
        uint32_t flags,
        void* window_handle,
        void*& surface,
        void*& swap_chain,
        array<void*, 3>& backbuffer_textures,
        array<void*, 3>& backbuffer_texture_views,
        array<std::shared_ptr<RHI_Semaphore>, 3>& image_acquired_semaphore
    )
        {
            RHI_Context* rhi_context = rhi_device->GetContextRhi();

            // Verify window handle
            const HWND hwnd = static_cast<HWND>(window_handle);
            SP_ASSERT(hwnd != nullptr);
            SP_ASSERT(IsWindow(hwnd));

            // Create surface
            VkSurfaceKHR _surface = nullptr;
            {
                VkWin32SurfaceCreateInfoKHR create_info = {};
                create_info.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                create_info.hwnd                        = hwnd;
                create_info.hinstance                   = GetModuleHandle(nullptr);

                SP_ASSERT(vulkan_utility::error::check(
                    vkCreateWin32SurfaceKHR(
                        rhi_device->GetContextRhi()->instance,
                        &create_info,
                        nullptr,
                        &_surface
                    )) && "Failed to created Win32 surface");

                VkBool32 present_support = false;

                SP_ASSERT(
                    vulkan_utility::error::check(vkGetPhysicalDeviceSurfaceSupportKHR(
                        rhi_context->device_physical,
                            rhi_device->GetQueueIndex(RHI_Queue_Type::Graphics),
                            _surface,
                            &present_support
                    )) && "Failed to get physical device surface support");

                SP_ASSERT(present_support && "The device does not support this kind of surface");
            }

            // Get surface capabilities
            VkSurfaceCapabilitiesKHR capabilities = vulkan_utility::surface::capabilities(_surface);

            // Compute extent
            *width            = Math::Helper::Clamp(*width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            *height           = Math::Helper::Clamp(*height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            VkExtent2D extent = { *width, *height };

            // Detect surface format and color space
            vulkan_utility::surface::detect_format_and_color_space(_surface, &rhi_context->surface_format, &rhi_context->surface_color_space);

            // Swap chain
            VkSwapchainKHR _swap_chain;
            {
                VkSwapchainCreateInfoKHR create_info = {};
                create_info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                create_info.surface                  = _surface;
                create_info.minImageCount            = buffer_count;
                create_info.imageFormat              = rhi_context->surface_format;
                create_info.imageColorSpace          = rhi_context->surface_color_space;
                create_info.imageExtent              = extent;
                create_info.imageArrayLayers         = 1;
                create_info.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                uint32_t queueFamilyIndices[] = { rhi_device->GetQueueIndex(RHI_Queue_Type::Compute), rhi_device->GetQueueIndex(RHI_Queue_Type::Graphics) };
                if (queueFamilyIndices[0] != queueFamilyIndices[1])
                {
                    create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
                    create_info.queueFamilyIndexCount = 2;
                    create_info.pQueueFamilyIndices   = queueFamilyIndices;
                }
                else
                {
                    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
                    create_info.queueFamilyIndexCount = 0;
                    create_info.pQueueFamilyIndices   = nullptr;
                }

                create_info.preTransform   = capabilities.currentTransform;
                create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                create_info.presentMode    = vulkan_utility::surface::set_present_mode(_surface, flags);
                create_info.clipped        = VK_TRUE;
                create_info.oldSwapchain   = nullptr;

                SP_ASSERT(vulkan_utility::error::check(vkCreateSwapchainKHR(rhi_context->device, &create_info, nullptr, &_swap_chain)) && "Failed to create swapchain");
            }

            // Images
            uint32_t image_count;
            vector<VkImage> images;
            {
                // Get
                vkGetSwapchainImagesKHR(rhi_context->device, _swap_chain, &image_count, nullptr);
                images.resize(image_count);
                vkGetSwapchainImagesKHR(rhi_context->device, _swap_chain, &image_count, images.data());

                // Transition layouts to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                if (VkCommandBuffer cmd_buffer = vulkan_utility::command_buffer_immediate::begin(RHI_Queue_Type::Graphics))
                {
                    for (uint32_t i = 0; i < static_cast<uint32_t>(images.size()); i++)
                    {
                        vulkan_utility::image::set_layout(reinterpret_cast<void*>(cmd_buffer), reinterpret_cast<void*>(images[i]), VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 1, layouts[i], RHI_Image_Layout::Color_Attachment_Optimal);
                        layouts[i] = RHI_Image_Layout::Color_Attachment_Optimal;
                    }

                    // End/flush
                    vulkan_utility::command_buffer_immediate::end(RHI_Queue_Type::Graphics);
                }
            }

            // Image views
            {
                for (uint32_t i = 0; i < image_count; i++)
                {
                    backbuffer_textures[i] = static_cast<void*>(images[i]);

                    // Name the image
                    vulkan_utility::debug::set_name(images[i], string(string("swapchain_image_") + to_string(i)).c_str());

                    SP_ASSERT(
                        vulkan_utility::image::view::create(
                            static_cast<void*>(images[i]),
                            backbuffer_texture_views[i],
                            VK_IMAGE_VIEW_TYPE_2D,
                            rhi_context->surface_format,
                            VK_IMAGE_ASPECT_COLOR_BIT,
                            0, 1, 0, 1
                        ) && "Failed to create image view");
                }
            }

            surface    = static_cast<void*>(_surface);
            swap_chain = static_cast<void*>(_swap_chain);

            // Semaphores
            for (uint32_t i = 0; i < buffer_count; i++)
            {
                image_acquired_semaphore[i] = make_shared<RHI_Semaphore>(rhi_device, false, (string("swapchain_image_acquired_semaphore_") + to_string(i)).c_str());
            }
        }
    
    static void swapchain_destroy(
        RHI_Device* rhi_device,
        uint8_t buffer_count,
        void*& surface,
        void*& swap_chain,
        array<void*, 3>& image_views,
        array<std::shared_ptr<RHI_Semaphore>, 3>& image_acquired_semaphore
    )
    {
        RHI_Context* rhi_context = rhi_device->GetContextRhi();

        // Semaphores
        image_acquired_semaphore.fill(nullptr);
    
        // Image views
        vulkan_utility::image::view::destroy(image_views);
    
        // Swap chain view
        if (swap_chain)
        {
            vkDestroySwapchainKHR(rhi_context->device, static_cast<VkSwapchainKHR>(swap_chain), nullptr);
            swap_chain = nullptr;
        }
    
        // Surface
        if (surface)
        {
            vkDestroySurfaceKHR(rhi_context->instance, static_cast<VkSurfaceKHR>(surface), nullptr);
            surface = nullptr;
        }
    }

    RHI_SwapChain::RHI_SwapChain(
        void* window_handle,
        const shared_ptr<RHI_Device>& rhi_device,
        const uint32_t width,
        const uint32_t height,
        const RHI_Format format,
        const uint32_t buffer_count,
        const uint32_t flags,
        const char* name
    )
    {
        // Verify resolution
        if (!rhi_device->IsValidResolution(width, height))
        {
            LOG_WARNING("%dx%d is an invalid resolution", width, height);
            return;
        }

        m_semaphore_image_acquired.fill(nullptr);
        m_backbuffer_resource.fill(nullptr);
        m_backbuffer_resource_view.fill(nullptr);

        // Copy parameters
        m_format        = format;
        m_rhi_device    = rhi_device.get();
        m_buffer_count  = buffer_count;
        m_width         = width;
        m_height        = height;
        m_window_handle = window_handle;
        m_flags         = flags;
        m_object_name   = name;

        swapchain_create
        (
            m_rhi_device,
            &m_width,
            &m_height,
            m_buffer_count,
            m_format,
            m_layouts,
            m_flags,
            m_window_handle,
            m_surface,
            m_resource,
            m_backbuffer_resource,
            m_backbuffer_resource_view,
            m_semaphore_image_acquired
        );

        AcquireNextImage();
    }

    RHI_SwapChain::~RHI_SwapChain()
    {
        // Wait in case any command buffer is still in use
        m_rhi_device->QueueWaitAll();

        // Resources
        swapchain_destroy
        (
            m_rhi_device,
            m_buffer_count,
            m_surface,
            m_resource,
            m_backbuffer_resource_view,
            m_semaphore_image_acquired
        );
    }

    bool RHI_SwapChain::Resize(const uint32_t width, const uint32_t height, const bool force /*= false*/)
    {
        // Validate resolution
        m_present_enabled = m_rhi_device->IsValidResolution(width, height);

        if (!m_present_enabled)
        {
            // Return true as when minimizing, a resolution
            // of 0,0 can be passed in, and this is fine.
            return false;
        }

        // Only resize if needed
        if (!force)
        {
            if (m_width == width && m_height == height)
                return false;
        }

        // Wait in case any command buffer is still in use
        m_rhi_device->QueueWaitAll();

        // Save new dimensions
        m_width  = width;
        m_height = height;

        // Destroy previous swap chain
        swapchain_destroy
        (
            m_rhi_device,
            m_buffer_count,
            m_surface,
            m_resource,
            m_backbuffer_resource_view,
            m_semaphore_image_acquired
        );

        // Create the swap chain with the new dimensions
        swapchain_create
        (
            m_rhi_device,
            &m_width,
            &m_height,
            m_buffer_count,
            m_format,
            m_layouts,
            m_flags,
            m_window_handle,
            m_surface,
            m_resource,
            m_backbuffer_resource,
            m_backbuffer_resource_view,
            m_semaphore_image_acquired
        );

        // The pipeline state used by the pipeline will now be invalid since it's referring to a destroyed swap chain view.
        // By generating a new ID, the pipeline cache will automatically generate a new pipeline for this swap chain.
        m_object_id = GenerateObjectId();

        return true;
    }

    void RHI_SwapChain::AcquireNextImage()
    {
        SP_ASSERT(m_present_enabled && "No need to acquire next image when presenting is disabled");

        // Return if the swapchain has a single buffer and it has already been acquired
        if (m_buffer_count == 1 && m_index_image_acquired != numeric_limits<uint32_t>::max())
            return;

        // Get signal semaphore
        m_semaphore_index = (m_semaphore_index + 1) % m_buffer_count;
        RHI_Semaphore* signal_semaphore = m_semaphore_image_acquired[m_semaphore_index].get();

        // Reset semaphore if it wasn't waited for
        if (signal_semaphore->GetState() != RHI_Semaphore_State::Idle)
        {
            LOG_INFO("Reseting signal semaphore...");
            signal_semaphore->Reset();
        }

        // Acquire next image
        VkResult result = vkAcquireNextImageKHR(
            m_rhi_device->GetContextRhi()->device,                     // device
            static_cast<VkSwapchainKHR>(m_resource),                   // swapchain
            numeric_limits<uint64_t>::max(),                           // timeout
            static_cast<VkSemaphore>(signal_semaphore->GetResource()), // signal semaphore
            nullptr,                                                   // signal fence
            &m_index_image_acquired                                             // pImageIndex
        );

        // Recreate swapchain with different size (if needed)
        if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR))
        {
            LOG_INFO("Outdated swapchain, recreating...");

            if (!Resize(m_width, m_height, true))
            {
                LOG_ERROR("Failed to resize swapchain");
                return;
            }

            AcquireNextImage();
            return;
        }

        SP_ASSERT(result == VK_SUCCESS && "Failed to acquire next image");

        // Update semaphore state
        signal_semaphore->SetState(RHI_Semaphore_State::Signaled);
    }

    void RHI_SwapChain::Present()
    {
        SP_ASSERT(m_resource != nullptr && "Can't present, the swapchain has not been initialised");
        SP_ASSERT(m_present_enabled && "Can't present, presenting has been disabled");

        // Get the semaphores that present should wait for
        static vector<RHI_Semaphore*> wait_semaphores;
        {
            wait_semaphores.clear();

            // The first is simply the image acquired semaphore
            wait_semaphores.emplace_back(m_semaphore_image_acquired[m_semaphore_index].get());

            // The others are all the command lists
            const vector<shared_ptr<RHI_CommandPool>>& cmd_pools = m_rhi_device->GetCommandPools();
            for (shared_ptr<RHI_CommandPool> cmd_pool : cmd_pools)
            {
                // Is this command pool presenting to this swap chain ? If so, wait on those semaphores
                if (m_object_id == cmd_pool->GetSwapchainId())
                {
                    RHI_Semaphore* semaphore = cmd_pool->GetCurrentCommandList()->GetSemaphoreProccessed();

                    // Command lists can be discarded (when they reference destroyed memory).
                    // In cases like that, the command lists are not submitted and as a result, the semaphore won't be signaled.
                    if (semaphore->GetState() == RHI_Semaphore_State::Signaled)
                    {
                        wait_semaphores.emplace_back(semaphore);
                    }
                }
            }
        }

        // Present
        if (!m_rhi_device->QueuePresent(m_resource, &m_index_image_acquired, wait_semaphores))
        {
            LOG_ERROR("Failed to present");
            return;
        }

        // Acquire next image
        AcquireNextImage();
    }

    void RHI_SwapChain::SetLayout(const RHI_Image_Layout& layout, RHI_CommandList* cmd_list)
    {
        if (m_layouts[m_index_image_acquired] == layout)
            return;

        vulkan_utility::image::set_layout(
            reinterpret_cast<void*>(cmd_list->GetResource()),
            reinterpret_cast<void*>(m_backbuffer_resource[m_index_image_acquired]),
            VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 1,
            m_layouts[m_index_image_acquired],
            layout
        );

        m_layouts[m_index_image_acquired] = layout;
    }
}
