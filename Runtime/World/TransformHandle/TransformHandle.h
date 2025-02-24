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

//= INCLUDES =============================
#include <memory>
#include "TransformOperator.h"
#include "../../Core/SpartanDefinitions.h"
#include <unordered_map>
//========================================

namespace Spartan
{
    class World;
    class Input;
    class Camera;
    class Context;
    class Entity;
    class RHI_IndexBuffer;
    class RHI_VertexBuffer;

    class SPARTAN_CLASS TransformHandle
    {
    public:
        TransformHandle(Context* context);
        ~TransformHandle() = default;

        // Ticks the gizmo and returns true if it needs to be queried for rendering
        bool Tick(Camera* camera, float handle_size);

        std::weak_ptr<Spartan::Entity> SetSelectedEntity(const std::shared_ptr<Entity>& entity);
        uint32_t GetIndexCount();
        const RHI_VertexBuffer* GetVertexBuffer();
        const RHI_IndexBuffer* GetIndexBuffer();
        const TransformOperator* GetHandle();
        bool DrawXYZ()              const { return m_type == TransformHandleType::Scale; }
        bool IsEditing()            const { return m_is_editing; }
        Entity* GetSelectedEntity() const { return m_entity_selected.lock().get(); }
        
    private:
        bool m_needs_to_render = false;
        bool m_is_editing      = false;

        std::weak_ptr<Entity> m_entity_selected;
        std::unordered_map<TransformHandleType, std::shared_ptr<TransformOperator>> m_transform_operator;
        TransformHandleType m_type = TransformHandleType::Unknown;
        TransformHandleSpace m_space;
        Context* m_context = nullptr;
        Input* m_input     = nullptr;
        World* m_world     = nullptr;
    };
}
