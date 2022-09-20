#include "muli/contact.h"
#include "muli/block_solver.h"
#include "muli/contact_solver.h"
#include "muli/world.h"

namespace muli
{

extern DetectionFunction* DetectionFunctionMap[RigidBody::Shape::ShapeCount][RigidBody::Shape::ShapeCount];

Contact::Contact(RigidBody* _bodyA, RigidBody* _bodyB, const WorldSettings& _settings)
    : Constraint(_bodyA, _bodyB, _settings)
{
    muliAssert(bodyA->GetShape() >= bodyB->GetShape());

    manifold.numContacts = 0;

    beta = settings.POSITION_CORRECTION_BETA;
    restitution = MixRestitution(bodyA->restitution, bodyB->restitution);
    friction = MixFriction(bodyA->friction, bodyB->friction);

    collisionDetectionFunction = DetectionFunctionMap[bodyA->GetShape()][bodyB->GetShape()];
    muliAssert(collisionDetectionFunction != nullptr);
}

void Contact::Update()
{
    ContactManifold oldManifold = manifold;
    float oldNormalImpulse[MAX_CONTACT_POINT];
    float oldTangentImpulse[MAX_CONTACT_POINT];

    bool wasTouching = touching;
    touching = collisionDetectionFunction(bodyA, bodyB, &manifold);

    for (uint32 i = 0; i < MAX_CONTACT_POINT; i++)
    {
        oldNormalImpulse[i] = normalSolvers[i].impulseSum;
        normalSolvers[i].impulseSum = 0.0f;
        oldTangentImpulse[i] = tangentSolvers[i].impulseSum;
        tangentSolvers[i].impulseSum = 0.0f;
    }

    if (!touching)
    {
        return;
    }

    // Warm start the contact solver
    for (uint32 n = 0; n < manifold.numContacts; n++)
    {
        uint32 o = 0;
        for (; o < oldManifold.numContacts; o++)
        {
            if (manifold.contactPoints[n].id == oldManifold.contactPoints[o].id)
            {
                break;
            }
        }

        if (o < oldManifold.numContacts)
        {
            normalSolvers[n].impulseSum = oldNormalImpulse[o];
            tangentSolvers[n].impulseSum = oldTangentImpulse[o];

            persistent = true;
        }
    }
}

void Contact::Prepare()
{
    for (uint32 i = 0; i < manifold.numContacts; i++)
    {
        normalSolvers[i].Prepare(this, i, manifold.contactNormal, ContactSolver::Type::Normal);
        tangentSolvers[i].Prepare(this, i, manifold.contactTangent, ContactSolver::Type::Tangent);
        positionSolvers[i].Prepare(this, i);
    }

    if (manifold.numContacts == 2 && settings.BLOCK_SOLVE)
    {
        blockSolver.Prepare(this);
    }
}

void Contact::SolveVelocityConstraint()
{
    // Solve tangential constraint first
    for (uint32 i = 0; i < manifold.numContacts; i++)
    {
        tangentSolvers[i].Solve(&normalSolvers[i]);
    }

    if (manifold.numContacts == 1 || !settings.BLOCK_SOLVE)
    {
        for (uint32 i = 0; i < manifold.numContacts; i++)
        {
            normalSolvers[i].Solve();
        }
    }
    else // Solve two contact constraint in one shot using block solver
    {
        blockSolver.Solve();
    }
}

bool Contact::SolvePositionConstraint()
{
    bool solved = true;

    cLinearImpulseA = { 0.0f, 0.0f };
    cAngularImpulseA = 0.0f;
    cLinearImpulseB = { 0.0f, 0.0f };
    cAngularImpulseB = 0.0f;

    // Solve position constraint
    for (uint32 i = 0; i < manifold.numContacts; i++)
    {
        solved &= positionSolvers[i].Solve();
    }

    manifold.bodyA->transform.position += manifold.bodyA->invMass * cLinearImpulseA;
    manifold.bodyA->transform.rotation += manifold.bodyA->invInertia * cAngularImpulseA;
    manifold.bodyB->transform.position += manifold.bodyB->invMass * cLinearImpulseB;
    manifold.bodyB->transform.rotation += manifold.bodyB->invInertia * cAngularImpulseB;

    return solved;
}

} // namespace muli
