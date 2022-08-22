#pragma once

#include "common.h"
#include "simplex.h"

namespace spe
{
struct ClosestEdgeInfo
{
    size_t index;
    float distance;
    glm::vec2 normal;
};

class Polytope
{
public:
    std::vector<glm::vec2> vertices;

    Polytope(const Simplex& simplex);

    ClosestEdgeInfo GetClosestEdge() const;
    size_t Count() const;
};

inline size_t Polytope::Count() const
{
    return vertices.size();
}

}