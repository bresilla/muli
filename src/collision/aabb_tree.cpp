#include "muli/aabb_tree.h"
#include "muli/growable_array.h"
#include "muli/util.h"

namespace muli
{

AABBTree::AABBTree()
    : nodeID{ 0 }
    , root{ muliNullNode }
    , nodeCapacity{ 32 }
    , nodeCount{ 0 }
{
    nodes = (Node*)muli::Alloc(nodeCapacity * sizeof(Node));
    memset(nodes, 0, nodeCapacity * sizeof(Node));

    // Build a linked list for the free list.
    for (int32 i = 0; i < nodeCapacity - 1; ++i)
    {
        nodes[i].next = i + 1;
    }
    nodes[nodeCapacity - 1].next = muliNullNode;
    freeList = 0;
}

AABBTree::~AABBTree() noexcept
{
    muli::Free(nodes);
    root = muliNullNode;
    nodeCount = 0;
}

AABBTree::AABBTree(AABBTree&& other) noexcept
{
    nodeID = other.nodeID;
    root = other.root;

    nodes = other.nodes;
    nodeCount = other.nodeCount;
    nodeCapacity = other.nodeCapacity;

    freeList = other.freeList;

    other.nodeID = 0;
    other.root = muliNullNode;

    other.nodes = nullptr;
    other.nodeCount = 0;
    other.nodeCapacity = 0;

    other.freeList = muliNullNode;
}

AABBTree& AABBTree::operator=(AABBTree&& other) noexcept
{
    muliAssert(this != &other);

    muli::Free(nodes);

    nodeID = other.nodeID;
    root = other.root;

    nodes = other.nodes;
    nodeCount = other.nodeCount;
    nodeCapacity = other.nodeCapacity;

    freeList = other.freeList;

    other.nodeID = 0;
    other.root = muliNullNode;

    other.nodes = nullptr;
    other.nodeCount = 0;
    other.nodeCapacity = 0;

    other.freeList = muliNullNode;

    return *this;
}

NodeProxy AABBTree::InsertLeaf(NodeProxy leaf)
{
    muliAssert(0 <= leaf && leaf < nodeCapacity);
    muliAssert(nodes[leaf].IsLeaf());

    if (root == muliNullNode)
    {
        root = leaf;
        return leaf;
    }

    AABB aabb = nodes[leaf].aabb;

    // Find the best sibling for the new leaf

#if 1
    NodeProxy bestSibling = root;
    float bestCost = SAH(AABB::Union(nodes[root].aabb, aabb));

    // Candidate node with inherited cost
    struct Candidate
    {
        NodeProxy node;
        float inheritedCost;
    };

    GrowableArray<Candidate, 256> stack;
    stack.EmplaceBack(root, 0.0f);

    while (stack.Count() != 0)
    {
        NodeProxy current = stack.Back().node;
        float inheritedCost = stack.Back().inheritedCost;
        stack.PopBack();

        AABB combined = AABB::Union(nodes[current].aabb, aabb);
        float directCost = SAH(combined);

        float cost = directCost + inheritedCost;
        if (cost < bestCost)
        {
            bestCost = cost;
            bestSibling = current;
        }

        inheritedCost += directCost - SAH(nodes[current].aabb);

        float lowerBoundCost = SAH(aabb) + inheritedCost;
        if (lowerBoundCost < bestCost)
        {
            if (nodes[current].IsLeaf() == false)
            {
                stack.EmplaceBack(nodes[current].child1, inheritedCost);
                stack.EmplaceBack(nodes[current].child2, inheritedCost);
            }
        }
    }
#else
    // O(log n)
    // This method is faster when inserting a new node, but builds a slightly bad quality tree.
    NodeProxy bestSibling = root;
    while (nodes[bestSibling].IsLeaf() == false)
    {
        NodeProxy child1 = nodes[bestSibling].child1;
        NodeProxy child2 = nodes[bestSibling].child2;

        float area = SAH(nodes[bestSibling].aabb);
        AABB combined = AABB::Union(nodes[bestSibling].aabb, aabb);
        float combinedArea = SAH(combined);

        float cost = combinedArea;
        float inheritanceCost = combinedArea - area;

        float cost1;
        if (nodes[child1].IsLeaf())
        {
            cost1 = SAH(AABB::Union(nodes[child1].aabb, aabb)) + inheritanceCost;
        }
        else
        {
            float newArea = SAH(AABB::Union(nodes[child1].aabb, aabb));
            float oldArea = SAH(nodes[child1].aabb);
            cost1 = (newArea - oldArea) + inheritanceCost; // Lower bound cost required when descending down to child1
        }

        float cost2;
        if (nodes[child2].IsLeaf())
        {
            cost2 = SAH(AABB::Union(nodes[child2].aabb, aabb)) + inheritanceCost;
        }
        else
        {
            float newArea = SAH(AABB::Union(nodes[child2].aabb, aabb));
            float oldArea = SAH(nodes[child2].aabb);
            cost2 = (newArea - oldArea) + inheritanceCost; // Lower bound cost required when descending down to child2
        }

        if (cost < cost1 && cost < cost2)
        {
            break;
        }

        if (cost1 < cost2)
        {
            bestSibling = child1;
        }
        else
        {
            bestSibling = child2;
        }
    }
#endif

    // Create a new parent
    NodeProxy oldParent = nodes[bestSibling].parent;
    NodeProxy newParent = AllocateNode();
    nodes[newParent].aabb = AABB::Union(aabb, nodes[bestSibling].aabb);
    nodes[newParent].data = nullptr;
    nodes[newParent].parent = oldParent;

    if (oldParent != muliNullNode)
    {
        if (nodes[oldParent].child1 == bestSibling)
        {
            nodes[oldParent].child1 = newParent;
        }
        else
        {
            nodes[oldParent].child2 = newParent;
        }

        nodes[newParent].child1 = bestSibling;
        nodes[newParent].child2 = leaf;
        nodes[bestSibling].parent = newParent;
        nodes[leaf].parent = newParent;
    }
    else
    {
        nodes[newParent].child1 = bestSibling;
        nodes[newParent].child2 = leaf;
        nodes[bestSibling].parent = newParent;
        nodes[leaf].parent = newParent;
        root = newParent;
    }

    // Walk back up the tree refitting ancestors' AABB and applying rotations
    NodeProxy ancestor = nodes[leaf].parent;
    while (ancestor != muliNullNode)
    {
        NodeProxy child1 = nodes[ancestor].child1;
        NodeProxy child2 = nodes[ancestor].child2;

        nodes[ancestor].aabb = AABB::Union(nodes[child1].aabb, nodes[child2].aabb);

        Rotate(ancestor);

        ancestor = nodes[ancestor].parent;
    }

    return leaf;
}

void AABBTree::RemoveLeaf(NodeProxy leaf)
{
    muliAssert(0 <= leaf && leaf < nodeCapacity);
    muliAssert(nodes[leaf].IsLeaf());

    NodeProxy parent = nodes[leaf].parent;

    if (parent != muliNullNode) // node is not root
    {
        NodeProxy sibling = nodes[parent].child1 == leaf ? nodes[parent].child2 : nodes[parent].child1;

        if (nodes[parent].parent != muliNullNode) // sibling has grandparent
        {
            nodes[sibling].parent = nodes[parent].parent;

            NodeProxy grandParent = nodes[parent].parent;
            if (nodes[grandParent].child1 == parent)
            {
                nodes[grandParent].child1 = sibling;
            }
            else
            {
                nodes[grandParent].child2 = sibling;
            }
        }
        else // sibling has no grandparent
        {
            root = sibling;

            nodes[sibling].parent = muliNullNode;
        }

        FreeNode(parent);

        NodeProxy ancestor = nodes[sibling].parent;
        while (ancestor != muliNullNode)
        {
            NodeProxy child1 = nodes[ancestor].child1;
            NodeProxy child2 = nodes[ancestor].child2;

            nodes[ancestor].aabb = AABB::Union(nodes[child1].aabb, nodes[child2].aabb);

            ancestor = nodes[ancestor].parent;
        }
    }
    else // node is root
    {
        muliAssert(root == leaf);

        root = muliNullNode;
    }
}

NodeProxy AABBTree::CreateNode(Data* data, const AABB& aabb)
{
    NodeProxy newNode = AllocateNode();

    // Fatten the aabb
    nodes[newNode].aabb.max = aabb.max + aabb_margin;
    nodes[newNode].aabb.min = aabb.min - aabb_margin;
    nodes[newNode].data = data;
    nodes[newNode].parent = muliNullNode;
    nodes[newNode].moved = true;

    InsertLeaf(newNode);

    return newNode;
}

bool AABBTree::MoveNode(NodeProxy node, AABB aabb, const Vec2& displacement, bool forceMove)
{
    muliAssert(0 <= node && node < nodeCapacity);
    muliAssert(nodes[node].IsLeaf());

    const AABB& treeAABB = nodes[node].aabb;
    if (treeAABB.Contains(aabb) && forceMove == false)
    {
        return false;
    }

    Vec2 d = displacement * aabb_multiplier;

    if (d.x > 0.0f)
    {
        aabb.max.x += d.x;
    }
    else
    {
        aabb.min.x += d.x;
    }

    if (d.y > 0.0f)
    {
        aabb.max.y += d.y;
    }
    else
    {
        aabb.min.y += d.y;
    }

    // Fatten the aabb
    aabb.max += aabb_margin;
    aabb.min -= aabb_margin;

    RemoveLeaf(node);

    nodes[node].aabb = aabb;

    InsertLeaf(node);

    nodes[node].moved = true;

    return true;
}

void AABBTree::RemoveNode(NodeProxy node)
{
    muliAssert(0 <= node && node < nodeCapacity);
    muliAssert(nodes[node].IsLeaf());

    RemoveLeaf(node);
    FreeNode(node);
}

void AABBTree::Rotate(NodeProxy node)
{
    if (nodes[node].IsLeaf())
    {
        return;
    }

    if (nodes[node].parent == muliNullNode)
    {
        return;
    }

    NodeProxy parent = nodes[node].parent;

    NodeProxy sibling;
    if (nodes[parent].child1 == node)
    {
        sibling = nodes[parent].child2;
    }
    else
    {
        sibling = nodes[parent].child1;
    }

    int32 count = 2;
    float costDiffs[4];
    float nodeArea = SAH(nodes[node].aabb);

    costDiffs[0] = SAH(AABB::Union(nodes[sibling].aabb, nodes[nodes[node].child1].aabb)) - nodeArea;
    costDiffs[1] = SAH(AABB::Union(nodes[sibling].aabb, nodes[nodes[node].child2].aabb)) - nodeArea;

    if (nodes[sibling].IsLeaf() == false)
    {
        float siblingArea = SAH(nodes[sibling].aabb);
        costDiffs[2] = SAH(AABB::Union(nodes[node].aabb, nodes[nodes[sibling].child1].aabb)) - siblingArea;
        costDiffs[3] = SAH(AABB::Union(nodes[node].aabb, nodes[nodes[sibling].child2].aabb)) - siblingArea;

        count += 2;
    }

    int32 bestDiffIndex = 0;
    for (int32 i = 1; i < count; ++i)
    {
        if (costDiffs[i] < costDiffs[bestDiffIndex])
        {
            bestDiffIndex = i;
        }
    }

    // Rotate only if it reduce the suface area
    if (costDiffs[bestDiffIndex] < 0.0f)
    {
        // printf("Tree rotation occurred: %d\n", bestDiffIndex);

        switch (bestDiffIndex)
        {
        case 0:
        {
            // Swap(sibling, node->child2);
            if (nodes[parent].child1 == sibling)
            {
                nodes[parent].child1 = nodes[node].child2;
            }
            else
            {
                nodes[parent].child2 = nodes[node].child2;
            }

            nodes[nodes[node].child2].parent = parent;

            nodes[node].child2 = sibling;
            nodes[sibling].parent = node;

            nodes[node].aabb = AABB::Union(nodes[sibling].aabb, nodes[nodes[node].child1].aabb);
        }
        break;
        case 1:
        {
            // Swap(sibling, node->child1);
            if (nodes[parent].child1 == sibling)
            {
                nodes[parent].child1 = nodes[node].child1;
            }
            else
            {
                nodes[parent].child2 = nodes[node].child1;
            }

            nodes[nodes[node].child1].parent = parent;

            nodes[node].child1 = sibling;
            nodes[sibling].parent = node;

            nodes[node].aabb = AABB::Union(nodes[sibling].aabb, nodes[nodes[node].child2].aabb);
        }
        break;
        case 2:
        {
            // Swap(node, sibling->child2);
            if (nodes[parent].child1 == node)
            {
                nodes[parent].child1 = nodes[sibling].child2;
            }
            else
            {
                nodes[parent].child2 = nodes[sibling].child2;
            }

            nodes[nodes[sibling].child2].parent = parent;

            nodes[sibling].child2 = node;
            nodes[node].parent = sibling;

            nodes[sibling].aabb = AABB::Union(nodes[node].aabb, nodes[nodes[sibling].child2].aabb);
        }
        break;
        case 3:
        {
            // Swap(node, sibling->child1);
            if (nodes[parent].child1 == node)
            {
                nodes[parent].child1 = nodes[sibling].child1;
            }
            else
            {
                nodes[parent].child2 = nodes[sibling].child1;
            }

            nodes[nodes[sibling].child1].parent = parent;

            nodes[sibling].child1 = node;
            nodes[node].parent = sibling;

            nodes[sibling].aabb = AABB::Union(nodes[node].aabb, nodes[nodes[sibling].child1].aabb);
        }
        break;
        }
    }
}

void AABBTree::Swap(NodeProxy node1, NodeProxy node2)
{
    NodeProxy parent1 = nodes[node1].parent;
    NodeProxy parent2 = nodes[node2].parent;

    if (parent1 == parent2)
    {
        nodes[parent1].child1 = node2;
        nodes[parent1].child2 = node1;
        return;
    }

    if (nodes[parent1].child1 == node1)
    {
        nodes[parent1].child1 = node2;
    }
    else
    {
        nodes[parent1].child2 = node2;
    }
    nodes[node2].parent = parent1;

    if (nodes[parent2].child1 == node2)
    {
        nodes[parent2].child1 = node1;
    }
    else
    {
        nodes[parent2].child2 = node1;
    }
    nodes[node1].parent = parent2;
}

void AABBTree::Traverse(const std::function<void(const Node*)>& callback) const
{
    if (root == muliNullNode)
    {
        return;
    }

    GrowableArray<NodeProxy, 256> stack;
    stack.EmplaceBack(root);

    while (stack.Count() != 0)
    {
        NodeProxy current = stack.PopBack();

        if (nodes[current].IsLeaf() == false)
        {
            stack.EmplaceBack(nodes[current].child1);
            stack.EmplaceBack(nodes[current].child2);
        }

        const Node* node = nodes + current;
        callback(node);
    }
}

void AABBTree::Query(const Vec2& point, const std::function<bool(NodeProxy, Data*)>& callback) const
{
    if (root == muliNullNode)
    {
        return;
    }

    GrowableArray<NodeProxy, 256> stack;
    stack.EmplaceBack(root);

    while (stack.Count() != 0)
    {
        NodeProxy current = stack.PopBack();

        if (nodes[current].aabb.TestPoint(point) == false)
        {
            continue;
        }

        if (nodes[current].IsLeaf())
        {
            bool proceed = callback(current, nodes[current].data);
            if (proceed == false)
            {
                return;
            }
        }
        else
        {
            stack.EmplaceBack(nodes[current].child1);
            stack.EmplaceBack(nodes[current].child2);
        }
    }
}

void AABBTree::Query(const AABB& aabb, const std::function<bool(NodeProxy, Data*)>& callback) const
{
    if (root == muliNullNode)
    {
        return;
    }

    GrowableArray<NodeProxy, 256> stack;
    stack.EmplaceBack(root);

    while (stack.Count() != 0)
    {
        NodeProxy current = stack.PopBack();

        if (nodes[current].aabb.TestOverlap(aabb) == false)
        {
            continue;
        }

        if (nodes[current].IsLeaf())
        {
            bool proceed = callback(current, nodes[current].data);
            if (proceed == false)
            {
                return;
            }
        }
        else
        {
            stack.EmplaceBack(nodes[current].child1);
            stack.EmplaceBack(nodes[current].child2);
        }
    }
}

void AABBTree::RayCast(const RayCastInput& input, const std::function<float(const RayCastInput&, Data*)>& callback) const
{
    Vec2 p1 = input.from;
    Vec2 p2 = input.to;
    float maxFraction = input.maxFraction;
    Vec2 radius(input.radius, input.radius);

    Vec2 d = p2 - p1;
    float length = d.NormalizeSafe();
    if (length == 0.0f)
    {
        return;
    }

    Vec2 perp = Cross(d, 1.0f); // separating axis
    Vec2 absPerp = Abs(perp);

    Vec2 end = p1 + maxFraction * (p2 - p1);
    AABB rayAABB;
    rayAABB.min = Min(p1, end) - radius;
    rayAABB.max = Max(p1, end) + radius;

    GrowableArray<NodeProxy, 256> stack;
    stack.EmplaceBack(root);

    while (stack.Count() > 0)
    {
        NodeProxy nodeIndex = stack.PopBack();
        if (nodeIndex == muliNullNode)
        {
            continue;
        }

        const Node* node = nodes + nodeIndex;
        if (node->aabb.TestOverlap(rayAABB) == false)
        {
            continue;
        }

        Vec2 center = (node->aabb.min + node->aabb.max) * 0.5f;
        Vec2 extents = (node->aabb.max - node->aabb.min) * 0.5f;

        float separation = Abs(Dot(perp, p1 - center)) - Dot(absPerp, extents);
        if (separation > radius.x) // Separating axis test
        {
            continue;
        }

        if (node->IsLeaf())
        {
            RayCastInput subInput;
            subInput.from = p1;
            subInput.to = p2;
            subInput.maxFraction = maxFraction;
            subInput.radius = input.radius;

            float newFraction = callback(subInput, node->data);
            if (newFraction == 0.0f)
            {
                return;
            }

            if (newFraction > 0.0f)
            {
                // Update ray AABB
                maxFraction = newFraction;
                Vec2 newEnd = p1 + maxFraction * (p2 - p1);
                rayAABB.min = Min(p1, newEnd) - radius;
                rayAABB.max = Max(p1, newEnd) + radius;
            }
        }
        else
        {
            stack.EmplaceBack(node->child1);
            stack.EmplaceBack(node->child2);
        }
    }
}

void AABBTree::Reset()
{
    nodeID = 0;
    root = muliNullNode;
    nodeCount = 0;
    memset(nodes, 0, nodeCapacity * sizeof(Node));

    // Build a linked list for the free list.
    for (int32 i = 0; i < nodeCapacity - 1; ++i)
    {
        nodes[i].next = i + 1;
    }
    nodes[nodeCapacity - 1].next = muliNullNode;
    freeList = 0;
}

NodeProxy AABBTree::AllocateNode()
{
    if (freeList == muliNullNode)
    {
        muliAssert(nodeCount == nodeCapacity);

        // Grow the node pool
        Node* oldNodes = nodes;
        nodeCapacity += nodeCapacity / 2;
        nodes = (Node*)muli::Alloc(nodeCapacity * sizeof(Node));
        memcpy(nodes, oldNodes, nodeCount * sizeof(Node));
        memset(nodes + nodeCount, 0, (nodeCapacity - nodeCount) * sizeof(Node));
        muli::Free(oldNodes);

        // Build a linked list for the free list.
        for (int32 i = nodeCount; i < nodeCapacity - 1; ++i)
        {
            nodes[i].next = i + 1;
        }
        nodes[nodeCapacity - 1].next = muliNullNode;
        freeList = nodeCount;
    }

    NodeProxy node = freeList;
    freeList = nodes[node].next;
    nodes[node].id = ++nodeID;
    nodes[node].parent = muliNullNode;
    nodes[node].child1 = muliNullNode;
    nodes[node].child2 = muliNullNode;
    nodes[node].moved = false;
    ++nodeCount;

    return node;
}

void AABBTree::FreeNode(NodeProxy node)
{
    muliAssert(0 <= node && node <= nodeCapacity);
    muliAssert(0 < nodeCount);

    nodes[node].id = 0;
    nodes[node].next = freeList;
    freeList = node;
    --nodeCount;
}

void AABBTree::Rebuild()
{
    // Rebuild the tree with bottom up approach

    NodeProxy* leaves = (NodeProxy*)muli::Alloc(nodeCount * sizeof(NodeProxy));
    int32 count = 0;

    // Build an array of leaf node
    for (int32 i = 0; i < nodeCapacity; ++i)
    {
        // Already in the free list
        if (nodes[i].id == 0)
        {
            continue;
        }

        // Clean the leaf
        if (nodes[i].IsLeaf())
        {
            nodes[i].parent = muliNullNode;

            leaves[count++] = i;
        }
        else
        {
            // Free the internal node
            FreeNode(i);
        }
    }

    while (count > 1)
    {
        float minCost = max_value;
        int32 minI = -1;
        int32 minJ = -1;

        // Find the best aabb pair
        for (int32 i = 0; i < count; ++i)
        {
            AABB aabbI = nodes[leaves[i]].aabb;

            for (int32 j = i + 1; j < count; ++j)
            {
                AABB aabbJ = nodes[leaves[j]].aabb;

                AABB combined = AABB::Union(aabbI, aabbJ);
                float cost = SAH(combined);

                if (cost < minCost)
                {
                    minCost = cost;
                    minI = i;
                    minJ = j;
                }
            }
        }

        NodeProxy index1 = leaves[minI];
        NodeProxy index2 = leaves[minJ];
        Node* child1 = nodes + index1;
        Node* child2 = nodes + index2;

        // Create a parent(internal) node
        NodeProxy parentIndex = AllocateNode();
        Node* parent = nodes + parentIndex;

        parent->child1 = index1;
        parent->child2 = index2;
        parent->aabb = AABB::Union(child1->aabb, child2->aabb);
        parent->parent = muliNullNode;

        child1->parent = parentIndex;
        child2->parent = parentIndex;

        leaves[minI] = parentIndex;

        leaves[minJ] = leaves[count - 1];
        --count;
    }

    root = leaves[0];
    muli::Free(leaves);
}

} // namespace muli