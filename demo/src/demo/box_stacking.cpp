#include "demo.h"

namespace muli
{

class BoxStacking : public Demo
{
public:
    BoxStacking()
    {
        settings.APPLY_GRAVITY = true;
        RigidBody* ground = world->CreateBox(100.0f, 0.4f, RigidBody::Type::Static);

        float start = 0.5f;
        float size = 0.3f;
        float gap = 0.25f;

        // float error = 0.015f;
        float error = 0.0f;

        for (uint32 i = 0; i < 20; i++)
        {
            RigidBody* b = world->CreateBox(size);
            b->SetPosition(LinearRand(-error, error), start + i * (size + gap));
        }
    }

    static Demo* Create()
    {
        return new BoxStacking();
    }
};

DemoFrame box_stacking{ "Box stacking", BoxStacking::Create };

} // namespace muli
