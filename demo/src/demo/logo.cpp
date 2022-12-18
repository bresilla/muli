#include "demo.h"

namespace muli
{

class Logo : public Demo
{
public:
    Logo(Game& game)
        : Demo(game)
    {
        RigidBody* ground = world->CreateBox(100.0f, 0.4f, RigidBody::Type::static_body);

        RigidBody* b = world->CreateEmptyBody();

        float offset = 0.5f;
        float radius = 0.12f;

        // M
        b->CreateCapsuleCollider(Vec2{ -4.0f + offset, 0.0f }, Vec2{ -4.0f + offset, 2.0f }, radius);
        b->CreateCapsuleCollider(Vec2{ -4.0f + offset, 2.0f }, Vec2{ -3.5f + offset, 1.0f }, radius);
        b->CreateCapsuleCollider(Vec2{ -3.5f + offset, 1.0f }, Vec2{ -3.0f + offset, 2.0f }, radius);
        b->CreateCapsuleCollider(Vec2{ -3.0f + offset, 2.0f }, Vec2{ -3.0f + offset, 0.0f }, radius);

        // U
        b->CreateCapsuleCollider(Vec2{ -2.0f + offset, 2.0f }, Vec2{ -2.0f + offset, 0.2f }, radius);
        b->CreateCapsuleCollider(Vec2{ -1.0f + offset, 0.2f }, Vec2{ -1.0f + offset, 2.0f }, radius);
        b->CreateCapsuleCollider(Vec2{ -2.0f + offset, 0.2f }, Vec2{ -1.8f + offset, 0.0f }, radius);
        b->CreateCapsuleCollider(Vec2{ -1.2f + offset, 0.0f }, Vec2{ -1.0f + offset, 0.2f }, radius);
        b->CreateCapsuleCollider(Vec2{ -1.8f + offset, 0.0f }, Vec2{ -1.2f + offset, 0.0f }, radius);

        // L
        b->CreateCapsuleCollider(Vec2{ 0.0f + offset, 0.0f }, Vec2{ 0.0f + offset, 2.0f }, radius);
        b->CreateCapsuleCollider(Vec2{ 0.0f + offset, 0.0f }, Vec2{ 1.0f + offset, 0.0f }, radius);

        // I
        b->CreateCapsuleCollider(Vec2{ 2.0f + offset, 2.0f }, Vec2{ 3.0f + offset, 2.0f }, radius);
        b->CreateCapsuleCollider(Vec2{ 2.0f + offset, 0.0f }, Vec2{ 3.0f + offset, 0.0f }, radius);
        b->CreateCapsuleCollider(Vec2{ 2.5f + offset, 0.0f }, Vec2{ 2.5f + offset, 2.0f }, radius);

        b->SetPosition(0.0f, 4.0f);

        for (int32 i = 0; i < 100; ++i)
        {
            float size = LinearRand(0.1f, 0.3f);
            float r = LinearRand(default_radius, 0.06f);

            Vec2 pos = LinearRand(Vec2{ -5.0f }, Vec2{ 5.0f });
            pos.y += 30.0f;

            float angle = LinearRand(0.0f, pi);

            b = world->CreateRandomConvexPolygon(size, 6, RigidBody::Type::dynamic_body, r);
            b->SetPosition(pos);
            b->SetRotation(angle);
            b->UserFlag |= UserFlag::render_polygon_radius;
        }
    }

    static Demo* Create(Game& game)
    {
        return new Logo(game);
    }
};

DemoFrame logo{ "Logo", Logo::Create };

} // namespace muli
