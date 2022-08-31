#pragma once

#include "block_solver.h"
#include "constraint.h"
#include "contact_solver.h"
#include "detection.h"

namespace spe
{
class RigidBody;
class Contact;

struct ContactEdge
{
    RigidBody* other;
    Contact* contact;
    ContactEdge* prev = nullptr;
    ContactEdge* next = nullptr;
};

class Contact : Constraint
{
    friend class World;
    friend class ContactManager;
    friend class BroadPhase;
    friend class ContactSolver;
    friend class BlockSolver;

public:
    Contact(RigidBody* _bodyA, RigidBody* _bodyB, const Settings& _settings);
    ~Contact() noexcept = default;

    void Update();
    Contact* GetNext() const;

    virtual void Prepare() override;
    virtual void Solve() override;

    const ContactManifold& GetContactManifold() const;

private:
    Contact* prev = nullptr;
    Contact* next = nullptr;

    ContactEdge nodeA;
    ContactEdge nodeB;

    bool touching = false;
    bool persistent = false;

    ContactManifold manifold;

    ContactSolver tangentContacts[2];
    ContactSolver normalContacts[2];
    BlockSolver blockSolver;
};

inline Contact::Contact(RigidBody* _bodyA, RigidBody* _bodyB, const Settings& _settings)
    : Constraint(_bodyA, _bodyB, _settings)
{
    manifold.numContacts = 0;
}

inline Contact* Contact::GetNext() const
{
    return next;
}

inline const ContactManifold& Contact::GetContactManifold() const
{
    return manifold;
}

} // namespace spe