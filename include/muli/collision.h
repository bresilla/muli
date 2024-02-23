#pragma once

#include "settings.h"
#include "simplex.h"

/*
 *           \        /         ↑ <- Contact normal
 *            \      /          |
 *    ---------\----/-------------------------------  <- Reference edge
 *              \  /
 *               \/  <- Incident point(Contact point)
 *
 */

namespace muli
{

constexpr int32 max_contact_point_count = 2;

class RigidBody;
class Shape;

// 64byte
struct ContactManifold
{
    Point contactPoints[max_contact_point_count];
    Point referencePoint;
    Vec2 contactNormal; // Contact normal is always pointing from bodyA to bodyB
    Vec2 contactTangent;
    float penetrationDepth;
    int32 contactCount;
    bool featureFlipped; // Set to true if shape b is incident body
};

// clang-format off
typedef bool CollideFunction(const Shape*, const Transform&,
                             const Shape*, const Transform&,
                             ContactManifold*);
                               
bool Collide(const Shape* a, const Transform& tfA,
             const Shape* b, const Transform& tfB,
             ContactManifold* manifold = nullptr);

struct GJKResult
{
    Simplex simplex;
    Vec2 direction;
    float distance;
};

bool GJK(const Shape* a, const Transform& tfA,
         const Shape* b, const Transform& tfB,
         GJKResult* result);

struct EPAResult
{
    Vec2 contactNormal;
    float penetrationDepth;
};

void EPA(const Shape* a, const Transform& tfA,
         const Shape* b, const Transform& tfB,
         const Simplex& simplex,
         EPAResult* result);

// clang-format on

} // namespace muli