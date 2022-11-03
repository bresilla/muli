#include "muli/line_joint.h"
#include "muli/world.h"

namespace muli
{

LineJoint::LineJoint(RigidBody* _bodyA,
                     RigidBody* _bodyB,
                     Vec2 _anchor,
                     Vec2 _dir,
                     const WorldSettings& _settings,
                     float _frequency,
                     float _dampingRatio,
                     float _jointMass)
    : Joint(Joint::Type::JointLine, _bodyA, _bodyB, _settings, _frequency, _dampingRatio, _jointMass)
{
    localAnchorA = MulT(bodyA->GetTransform(), _anchor);
    localAnchorB = MulT(bodyB->GetTransform(), _anchor);

    if (_dir.Length2() < FLT_EPSILON)
    {
        Vec2 d = MulT(bodyA->GetRotation(), (_bodyB->GetPosition() - _bodyA->GetPosition()).Normalized());
        localYAxis = Cross(1.0f, d);
    }
    else
    {
        localYAxis = MulT(bodyA->GetRotation(), Cross(1.0f, _dir.Normalized()));
    }
}

void LineJoint::Prepare()
{
    // Calculate Jacobian J and effective mass M
    // J = [-t^t, -(ra + d)×t, t^t, rb×t]
    // M = (J · M^-1 · J^t)^-1

    Vec2 ra = bodyA->GetRotation() * localAnchorA;
    Vec2 rb = bodyB->GetRotation() * localAnchorB;
    Vec2 pa = bodyA->GetPosition() + ra;
    Vec2 pb = bodyB->GetPosition() + rb;
    Vec2 d = pb - pa;

    t = bodyA->GetRotation() * localYAxis;
    sa = Cross(ra + d, t);
    sb = Cross(rb, t);

    // clang-format off
    float k = bodyA->invMass + bodyB->invMass
            + bodyA->invInertia * sa * sa
            + bodyB->invInertia * sb * sb
            + gamma;
    // clang-format on

    if (k != 0.0f)
    {
        m = 1.0f / k;
    }

    float error = Dot(d, t);

    bias = error * beta * settings.INV_DT;

    if (settings.WARM_STARTING)
    {
        ApplyImpulse(impulseSum);
    }
}

void LineJoint::SolveVelocityConstraint()
{
    // Calculate corrective impulse: Pc
    // Pc = J^t · λ (λ: lagrangian multiplier)
    // λ = (J · M^-1 · J^t)^-1 ⋅ -(J·v+b)

    float jv = Dot(t, bodyB->linearVelocity - bodyA->linearVelocity) + sb * bodyB->angularVelocity - sa * bodyA->angularVelocity;

    float lambda = m * -(jv + bias + impulseSum * gamma);

    ApplyImpulse(lambda);
    impulseSum += lambda;
}

void LineJoint::ApplyImpulse(float lambda)
{
    // V2 = V2' + M^-1 ⋅ Pc
    // Pc = J^t ⋅ λ

    Vec2 p = t * lambda;

    bodyA->linearVelocity -= p * bodyA->invMass;
    bodyA->angularVelocity -= lambda * sa * bodyA->invInertia;
    bodyB->linearVelocity += p * bodyB->invMass;
    bodyB->angularVelocity += lambda * sb * bodyB->invInertia;
}

} // namespace muli