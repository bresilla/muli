#pragma once

#include "aabb.h"
#include "aabbtree.h"
#include "common.h"

namespace spe
{

class BroadPhase
{
    friend class World;

public:
    BroadPhase(World& _world);
    ~BroadPhase() noexcept;
    void UpdateDynamicTree(float dt);
    void FindContacts(std::function<void(RigidBody* bodyA, RigidBody* bodyB)> callback);
    void Reset();
    void Add(RigidBody* body);
    void Remove(RigidBody* body);
    bool TestOverlap(RigidBody* bodyA, RigidBody* bodyB);

private:
    World& world;
    float margin = 0.1f;
    float velocityMultiplier = 3.0f;
    AABBTree tree;
};

inline BroadPhase::BroadPhase(World& _world)
    : world{ _world }
{
}

inline BroadPhase::~BroadPhase()
{
    Reset();
}

inline void BroadPhase::Reset()
{
    tree.Reset();
}

inline void BroadPhase::Add(RigidBody* body)
{
    AABB fatAABB = body->GetAABB();
    fatAABB.min -= margin;
    fatAABB.max += margin;

    tree.Insert(body, fatAABB);
}

inline void BroadPhase::Remove(RigidBody* body)
{
    tree.Remove(body);
}

inline bool BroadPhase::TestOverlap(RigidBody* bodyA, RigidBody* bodyB)
{
    return test_overlap_aabb(bodyA->node->aabb, bodyB->node->aabb);
}

} // namespace spe