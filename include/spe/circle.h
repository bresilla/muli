#pragma once

#include "util.h"
#include "rigidbody.h"

namespace spe
{
class Circle : public RigidBody
{
public:
    Circle(float radius, BodyType _type = Dynamic, float _density = DEFAULT_DENSITY);

    inline virtual void SetMass(float m) override;
    inline virtual void SetDensity(float d) override;

    inline float GetRadius() const;
    inline virtual float GetArea() const override final;

protected:
    float radius;
    float area;
};

inline void Circle::SetMass(float _mass)
{
    assert(_mass > 0);

    density = _mass / area;
    mass = _mass;
    invMass = 1.0f / mass;
    inertia = calculate_circle_inertia(radius, mass);
    invInertia = 1.0f / inertia;
}

inline void Circle::SetDensity(float _density)
{
    assert(density > 0);

    density = _density;
    mass = density * area;
    invMass = 1.0f / mass;
    inertia = calculate_circle_inertia(radius, mass);
    invInertia = 1.0f / inertia;
}

inline float Circle::GetRadius() const
{
    return radius;
}

inline float Circle::GetArea() const
{
    return area;
}

}