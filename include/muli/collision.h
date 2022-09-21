#pragma once

#include "common.h"
#include "contact_point.h"
#include "edge.h"
#include "settings.h"

/*
 *           \        /         ↑
 *            \      /          | <- Contact normal
 *    ---------\----/-------------------------------  <- Reference edge
 *              \  /
 *               \/  <- Incident point(Contact point)
 *
 */

namespace muli
{

class RigidBody;
class Polygon;

// 64byte
struct ContactManifold
{
    ContactPoint contactPoints[MAX_CONTACT_POINT];
    ContactPoint referencePoint;
    Vec2 contactNormal; // Contact normal is always pointing from a to b
    Vec2 contactTangent;
    float penetrationDepth;
    uint32 numContacts;
    bool featureFlipped;
};

typedef bool DetectionFunction(RigidBody*, RigidBody*, ContactManifold*);

bool DetectCollision(RigidBody* a, RigidBody* b, ContactManifold* out = nullptr);
bool TestPointInside(RigidBody* b, const Vec2& q);
float ComputeDistance(RigidBody* a, RigidBody* b);
float ComputeDistance(RigidBody* b, const Vec2& q);
Vec2 GetClosestPoint(RigidBody* b, const Vec2& q);
Edge GetIntersectingEdge(Polygon* p, const Vec2& dir);

} // namespace muli