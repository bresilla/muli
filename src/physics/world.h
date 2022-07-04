#pragma once

#include "../common.h"
#include "aabbtree.h"
#include "detection.h"
#include "rigidbody.h"
#include "contact_constraint.h"

namespace spe
{
    // Simulation settings
    struct Settings
    {
        float DT = 1.0f / 60.0f;
        float INV_DT = 60.0f;
        glm::vec2 GRAVITY = glm::vec2{ 0.0f, -10.0f };
        bool IMPULSE_ACCUMULATION = true;
        bool WARM_STARTING = true;
        bool POSITION_CORRECTION = true;
        float POSITION_CORRECTION_BETA = 0.2f;
        float PENETRATION_SLOP = 0.005f;
        float RESTITUTION_SLOP = 0.5f;
        bool APPLY_WARM_STARTING_THRESHOLD = true;
        float WARM_STARTING_THRESHOLD = 0.005f * 0.005f;
        bool BLOCK_SOLVE = true;
    };

    class World final
    {
    public:
        World(const Settings& simulationSettings);
        ~World() noexcept;

        World(const World&) noexcept = delete;
        World& operator=(const World&) noexcept = delete;

        World(World&&) noexcept = delete;
        World& operator=(World&&) noexcept = delete;

        void Update();
        void Reset();

        void Register(RigidBody* body);
        void Register(const std::vector<RigidBody*>& bodies);
        void Unregister(RigidBody* body);
        void Unregister(const std::vector<RigidBody*>& bodies);

        std::vector<RigidBody*> QueryPoint(const glm::vec2& point) const;
        std::vector<RigidBody*> QueryRegion(const AABB& region) const;

        const std::vector<RigidBody*>& GetBodies() const;
        const AABBTree& GetBVH() const;
        const std::vector<std::unique_ptr<ContactConstraint>>& GetContactConstraints() const;
    private:
        const Settings& settings;
        uint32_t uid{ 0 };

        // Dynamic AABB Tree for broad phase collision detection
        AABBTree tree{};
        // All registered rigid bodies
        std::vector<RigidBody*> bodies{};

        // Constraints to be solved
        std::vector<std::unique_ptr<ContactConstraint>> contactConstraints{};
        std::unordered_map<int32_t, ContactConstraint*> contactConstraintMap{};
        std::unordered_set<int32_t> passTestSet{};
    };
}