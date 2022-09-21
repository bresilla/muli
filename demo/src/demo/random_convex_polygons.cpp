#include "demo.h"

namespace muli
{

class RandomConvexPolygons : public Demo
{
public:
    RandomConvexPolygons()
    {
        settings.APPLY_GRAVITY = true;
        RigidBody* ground = world->CreateBox(100.0f, 0.4f, RigidBody::Type::Static);

        int rows = 12;
        float size = 0.25f;
        float xGap = 0.2f;
        float yGap = 0.15f;
        float xStart = -(rows - 1) * (size + xGap) / 2.0f;
        float yStart = 1.0f;

        for (int y = 0; y < rows; y++)
        {
            for (int x = 0; x < rows - y; x++)
            {
                Polygon* b = world->CreateRandomConvexPolygon(size, 6);
                b->SetPosition(xStart + y * (size + xGap) / 2 + x * (size + xGap), yStart + y * (size + yGap));
                b->SetLinearVelocity(b->GetPosition() * LinearRand(0.5f, 0.7f));
                b->SetFriction(LinearRand(0.2f, 1.0f));
            }
        }

        Capsule* pillar = world->CreateCapsule(4.0f, 0.1f, false, RigidBody::Type::Static);
        pillar->SetPosition(xStart - 0.2f, 3.0f);

        pillar = world->CreateCapsule(4.0f, 0.1f, false, RigidBody::Type::Static);
        pillar->SetPosition(-(xStart - 0.2f), 3.0f);

        // Capsule* c = world->CreateCapsule(4.0f, 0.1f, true, RigidBody::Type::Dynamic);
        // c->SetPosition(0.0f, 2.0f);

        // world->CreateGrabJoint(c, c->GetPosition(), c->GetPosition(), -1.0f);
    }

    static Demo* Create()
    {
        return new RandomConvexPolygons;
    }
};

DemoFrame random_convex_polygons{ "Random convex polygons", RandomConvexPolygons::Create };

} // namespace muli