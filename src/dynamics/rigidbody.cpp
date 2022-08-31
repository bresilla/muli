#include "spe/rigidbody.h"

namespace spe
{

RigidBody::RigidBody(BodyType _type)
    : Entity()
    , type{ std::move(_type) }
{
    if (type == Static)
    {
        density = FLT_MAX;
        mass = FLT_MAX;
        invMass = 0.0f;
        inertia = FLT_MAX;
        invInertia = 0.0f;
    }
    else
    {
        // This part is implemented by children.
    }
}

RigidBody::~RigidBody()
{
    if (moved) return;

    world = nullptr;
    id = 0;

    if (OnDestroy != nullptr) OnDestroy(this);
}

RigidBody::RigidBody(RigidBody&& _other) noexcept
{
    _other.moved = true;

    // private member
    world = _other.world;
    id = _other.id;
    islandID = _other.islandID;

    contactList = _other.contactList;
    jointIDs = std::move(_other.jointIDs);

    resting = _other.resting;
    sleeping = _other.sleeping;

    node = _other.node;
    _other.node = nullptr;

    // protected member
    force = _other.force;
    torque = _other.torque;

    linearVelocity = _other.linearVelocity;
    angularVelocity = _other.angularVelocity;

    density = _other.density;
    mass = _other.mass;
    invMass = _other.invMass;
    inertia = _other.inertia;
    invInertia = _other.invInertia;

    friction = _other.friction;
    restitution = _other.restitution;
    surfaceSpeed = _other.surfaceSpeed;

    type = _other.type;

    // public member
    OnDestroy = std::move(_other.OnDestroy);

    Entity::operator=(std::move(_other));
}

} // namespace spe