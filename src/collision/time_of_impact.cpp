#include "muli/time_of_impact.h"
#include "muli/settings.h"

namespace muli
{

struct SeparationFunction
{
    enum Type
    {
        points,
        edgeA,
        edgeB,
    };

    void Initialize(const ClosestFeatures& closestFeatures,
                    const Shape* _shapeA,
                    const Sweep& _sweepA,
                    const Shape* _shapeB,
                    const Sweep& _sweepB,
                    float t1)
    {
        muliAssert(0 < closestFeatures.count && closestFeatures.count < MAX_SIMPLEX_VERTEX_COUNT);

        shapeA = _shapeA;
        shapeB = _shapeB;

        sweepA = _sweepA;
        sweepB = _sweepB;

        Transform tfA, tfB;
        sweepA.GetTransform(t1, &tfA);
        sweepB.GetTransform(t1, &tfB);

        int32 count = closestFeatures.count;
        const ContactPoint* featuresA = closestFeatures.featuresA;
        const ContactPoint* featuresB = closestFeatures.featuresB;

        if (count == 1)
        {
            // Point A vs. Point B
            type = points;

            // separating axis in world space
            axis = featuresB[0].position - featuresA[0].position;
            axis.Normalize();
        }
        else if (featuresA[0].id == featuresB[1].id)
        {
            // Point A vs. Edge B
            type = edgeB;

            Vec2 localPointB0 = shapeB->GetVertex(featuresB[0].id);
            Vec2 localPointB1 = shapeB->GetVertex(featuresB[1].id);

            // separating axis in the frame of body B
            axis = Cross(localPointB1 - localPointB0, 1.0f);
            axis.Normalize();

            // world space face normal
            Vec2 normal = tfB.rotation * axis;

            Vec2 pointB = (featuresB[0].position + featuresB[1].position) * 0.5f;
            Vec2 pointA = featuresA[0].position;

            float separation = Dot(normal, pointA - pointB);
            if (separation < 0.0f)
            {
                axis = -axis;
            }
        }
        else
        {
            // Edge A vs. Point B
            type = edgeA;

            Vec2 localPointA0 = shapeA->GetVertex(featuresA[0].id);
            Vec2 localPointA1 = shapeA->GetVertex(featuresA[1].id);

            // separating axis in the frame of body A
            axis = Cross(localPointA1 - localPointA0, 1.0f);
            axis.Normalize();

            // world space face normal
            Vec2 normal = tfA.rotation * axis;

            Vec2 pointA = (featuresA[0].position + featuresA[1].position) * 0.5f;
            Vec2 pointB = featuresB[0].position;

            float separation = Dot(normal, pointB - pointA);
            if (separation < 0.0f)
            {
                axis = -axis;
            }
        }
    }

    float FindMinSeparation(float t, int32* idA, int32* idB) const
    {
        Transform tfA, tfB;
        sweepA.GetTransform(t, &tfA);
        sweepB.GetTransform(t, &tfB);

        switch (type)
        {
        case points:
        {
            // Separation axis is in world space and pointing from a to b
            Vec2 localAxisA = MulT(tfA.rotation, axis);
            Vec2 localAxisB = MulT(tfB.rotation, -axis);

            *idA = shapeA->Support(localAxisA).id;
            *idB = shapeB->Support(localAxisB).id;

            Vec2 localPointA = shapeA->GetVertex(*idA);
            Vec2 localPointB = shapeB->GetVertex(*idB);

            Vec2 pointA = tfA * localPointA;
            Vec2 pointB = tfB * localPointB;

            float separation = Dot(axis, pointB - pointA);
            return separation;
        }
        case edgeA:
        {
            // world space face normal and refernce point
            Vec2 normal = tfA.rotation * axis;
            Vec2 pointA = tfA * localPoint;

            Vec2 localAxisB = MulT(tfB.rotation, -normal);

            *idA = -1;
            *idB = shapeB->Support(localAxisB).id;

            Vec2 localPointB = shapeB->GetVertex(*idB);
            Vec2 pointB = tfB * localPointB;

            float separation = Dot(normal, pointB - pointA);
            return separation;
        }
        case edgeB:
        {
            // world space face normal and refernce point
            Vec2 normal = tfB.rotation * axis;
            Vec2 pointB = tfB * localPoint;

            Vec2 localAxisA = MulT(tfA.rotation, -normal);

            *idA = shapeA->Support(localAxisA).id;
            *idB = -1;

            Vec2 localPointA = shapeA->GetVertex(*idA);
            Vec2 pointA = tfA * localPointA;

            float separation = Dot(normal, pointA - pointB);
            return separation;
        }
        default:
            muliAssert(false);
            return 0.0f;
        }
    }

    float ComputeSeparation(int32 idA, int32 idB, float t) const
    {
        Transform tfA, tfB;
        sweepA.GetTransform(t, &tfA);
        sweepB.GetTransform(t, &tfB);

        switch (type)
        {
        case points:
        {
            Vec2 localPointA = shapeA->GetVertex(idA);
            Vec2 localPointB = shapeA->GetVertex(idB);

            Vec2 pointA = tfA * localPointA;
            Vec2 pointB = tfB * localPointB;

            float separation = Dot(axis, pointB - pointA);
            return separation;
        }
        case edgeA:
        {
            Vec2 normal = tfA.rotation * axis;

            Vec2 pointA = tfA * localPoint;

            Vec2 localPointB = shapeB->GetVertex(idB);
            Vec2 pointB = tfB * localPointB;

            float separation = Dot(normal, pointB - pointA);
            return separation;
        }
        case edgeB:
        {
            Vec2 normal = tfB.rotation * axis;

            Vec2 pointB = tfB * localPoint;

            Vec2 localPointA = shapeA->GetVertex(idA);
            Vec2 pointA = tfA * localPointA;

            float separation = Dot(normal, pointA - pointB);
            return separation;
        }
        default:
            muliAssert(false);
            return 0.0f;
        }
    }

    const Shape* shapeA;
    const Shape* shapeB;
    Sweep sweepA;
    Sweep sweepB;
    Type type;
    Vec2 localPoint;
    Vec2 axis;
};

constexpr int32 max_iteration = 20;

void FindTimeOfImpact(const TOIInput& input, TOIOutput* output)
{
    output->state = TOIOutput::unknown;
    output->t = input.tMax;

    const Shape* shapeA = input.shapeA;
    const Shape* shapeB = input.shapeB;

    Sweep sweepA = input.sweepA;
    Sweep sweepB = input.sweepB;

    sweepA.Normalize();
    sweepB.Normalize();

    float tMax = input.tMax;

    float r2 = shapeA->GetRadius() + shapeB->GetRadius();
    float target = Max(LINEAR_SLOP, r2 - 2.0f * LINEAR_SLOP);
    float tolerance = 0.25f * LINEAR_SLOP;
    muliAssert(target > tolerance);

    float t1 = 0.0f;
    int32 iteration = 0;

    ClosestFeatures cf;

    while (true)
    {
        Transform tfA, tfB;
        sweepA.GetTransform(t1, &tfA);
        sweepB.GetTransform(t1, &tfB);

        // Get the initial separation
        float distance = GetClosestFeatures(shapeA, tfA, shapeB, tfB, &cf);

        // Two shapes are overlapped at initial configuration
        if (distance == 0.0f)
        {
            // Failure!
            output->state = TOIOutput::overlapped;
            output->t = 0.0f;
            break;
        }

        if (distance < target + tolerance)
        {
            // Victory!
            output->state = TOIOutput::touching;
            output->t = t1;
            break;
        }

        // Initialize the separating axis
        SeparationFunction fcn;
        fcn.Initialize(cf, shapeA, sweepA, shapeB, sweepB, t1);

        // Compute the time of impact on the separating axis
        // We do this by successively resolving the deepest point
        bool done = false;
        float t2 = tMax;
        while (true)
        {
            // Find the deepest point at t2 and save them
            int32 idA, idB;
            float s2 = fcn.FindMinSeparation(t2, &idA, &idB);

            // Is the final configuration separated?
            if (s2 > target + tolerance)
            {
                // Victory!
                output->state = TOIOutput::separated;
                output->t = tMax;
                done = true;
                break;
            }

            // Has the separation reached tolerance?
            if (s2 > target - tolerance)
            {
                // Advance the sweeps
                t1 = t2;
                break;
            }

            // Compute the initial separation
            float s1 = fcn.ComputeSeparation(idA, idB, t1);

            // Check for initial overlap
            // This might happen if the root finder runs out of iterations
            if (s1 < target - tolerance)
            {
                output->state = TOIOutput::failed;
                output->t = t1;
                done = true;
                break;
            }

            // Check for touching
            if (s1 <= target + tolerance)
            {
                // Victory! t1 should hold the TOI (could be 0.0)
                output->state = TOIOutput::touching;
                output->t = t1;
                done = true;
                break;
            }

            // Compute 1D root of: f(x) - target = 0
            int32 i = 0;
            float a1 = t1;
            float a2 = t2;
            while (true)
            {
                float t;

                // Use a mix of secant method and bisection
                if (i & 1)
                {
                    // Secant method
                    t = a1 + (target - s1) * (a2 - a1) / (s2 - s1);
                }
                else
                {
                    // Bisection
                    t = 0.5f * (a1 + a2);
                }

                ++i;

                float s = fcn.ComputeSeparation(idA, idB, t);

                if (Abs(s - target) < tolerance)
                {
                    // t2 holds a tentative value for t1
                    t2 = t;
                    break;
                }

                // Ensure we continue to bracket the root
                if (s > target)
                {
                    a1 = t;
                    s1 = s;
                }
                else
                {
                    a2 = t;
                    s2 = s;
                }

                if (i == 50)
                {
                    break;
                }
            }
        }

        ++iteration;

        if (done)
        {
            break;
        }

        if (iteration == max_iteration)
        {
            // Root finder got stuck
            output->state = TOIOutput::failed;
            output->t = t1;
            break;
        }
    }
}

} // namespace muli