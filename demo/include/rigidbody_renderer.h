#pragma once

#include "common.h"
#include "renderer.h"
#include "rigidbody_shader.h"
#include "mesh.h"
#include "util.h"

namespace spe
{
class RigidBodyRenderer final : public Renderer
{
    friend class RigidBodyShader;

public:
    RigidBodyRenderer();

    virtual void Render() override;

    inline void SetProjectionMatrix(glm::mat4 _projMatrix);
    inline void SetViewMatrix(glm::mat4 _viewMatrix);

    inline void Register(RigidBody* body);
    inline void Register(const std::vector<RigidBody*>& bodies);
    inline void Unregister(RigidBody* body);
    inline void Unregister(const std::vector<RigidBody*>& bodies);

    glm::vec2 Pick(const glm::vec2& screenPos);

    inline void SetDrawOutlined(bool drawOutlineOnly);
    inline void Reset();

private:
    // All registered rigid bodies
    std::unique_ptr<RigidBodyShader> shader{};
    std::vector<std::pair<RigidBody*, std::unique_ptr<Mesh>>> bodiesAndMeshes{};

    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;

    bool drawOutlineOnly;
};

inline void RigidBodyRenderer::Register(RigidBody* body)
{
    bodiesAndMeshes.push_back(std::pair(body, generate_mesh_from_rigidbody(*body)));
}

inline void RigidBodyRenderer::Register(const std::vector<RigidBody*>& bodies)
{
    for (auto b : bodies)
    {
        Register(b);
    }
}

inline void RigidBodyRenderer::Unregister(RigidBody* body)
{
    size_t idx = bodiesAndMeshes.size();

    for (size_t i = 0; i < bodiesAndMeshes.size(); i++)
    {
        if (bodiesAndMeshes[i].first == body)
        {
            idx = i;
            break;
        }
    }

    if (idx < bodiesAndMeshes.size())
    {
        bodiesAndMeshes.erase(bodiesAndMeshes.begin() + idx);
    }
}

inline void RigidBodyRenderer::Unregister(const std::vector<RigidBody*>& bodies)
{
    for (size_t i = 0; i < bodies.size(); i++)
    {
        Unregister(bodies[i]);
    }
}

inline void RigidBodyRenderer::SetProjectionMatrix(glm::mat4 _projMatrix)
{
    shader->Use();
    shader->SetProjectionMatrix(std::move(_projMatrix));
}

inline void RigidBodyRenderer::SetViewMatrix(glm::mat4 _viewMatrix)
{
    shader->Use();
    shader->SetViewMatrix(std::move(_viewMatrix));
}

inline void RigidBodyRenderer::SetDrawOutlined(bool _drawOutlineOnly)
{
    drawOutlineOnly = std::move(_drawOutlineOnly);
}

inline void RigidBodyRenderer::Reset()
{
    bodiesAndMeshes.clear();
}

}