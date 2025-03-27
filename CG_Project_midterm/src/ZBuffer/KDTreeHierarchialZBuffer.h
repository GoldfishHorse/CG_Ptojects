#pragma once

#include "utils.h"
#include "Model.h"
#include "HierarchicalZBuffer.h"

#include <numeric>
#include <vector>

struct KDNode;
typedef KDNode *PKDNode;

/* KD-Tree 结点 */
struct KDNode : BoundingBox
{
public:
    /* 构建叶子结点 */
    KDNode(const BoundingBox &bbox)
        : BoundingBox(bbox),
          left(nullptr),
          right(nullptr)
    {
    }
    /* 构建非叶子结点 */
    KDNode(PKDNode left, PKDNode right)
        : BoundingBox{
              .minPoint = Vec3f{
                  std::numeric_limits<FLOAT>::max(),
                  std::numeric_limits<FLOAT>::max(),
                  std::numeric_limits<FLOAT>::max(),
              },
              .maxPoint = Vec3f{
                  std::numeric_limits<FLOAT>::lowest(),
                  std::numeric_limits<FLOAT>::lowest(),
                  std::numeric_limits<FLOAT>::lowest(),
              }},
          left(left), right(right)
    {
        ASSERT(!(left == nullptr && right == nullptr), "Invalid argument!");
        if (left != nullptr && right != nullptr)
        {
            /* concatenate faceIdces from left and right */
            /* https://stackoverflow.com/questions/3177241/what-is-the-best-way-to-concatenate-two-vectors */
            faceIdces.reserve(left->faceIdces.size() + right->faceIdces.size());
            faceIdces.insert(
                faceIdces.end(), left->faceIdces.begin(), left->faceIdces.end());
            faceIdces.insert(
                faceIdces.end(), right->faceIdces.begin(), right->faceIdces.end());

            /* extend bounding box */
            this->extend(*this->left);
            this->extend(*this->right);
        }
        else if (left != nullptr && right == nullptr)
        {
            faceIdces = left->faceIdces;
            this->extend(*this->left);
        }
        else if (left == nullptr && right != nullptr)
        {
            faceIdces = right->faceIdces;
            this->extend(*this->right);
        }
        return;
    }

    ~KDNode() { this->release(); }

    PKDNode left;
    PKDNode right;
    std::vector<size_t> faceIdces;

    /* 递归释放子结点 */
    void release()
    {
        if (left != nullptr)
        {
            left->release();
            delete left;
            left = nullptr;
        }
        if (right != nullptr)
        {
            right->release();
            delete right;
            right = nullptr;
        }
        return;
    }

    /** @brief 扩充自己的包围盒, 使其包含参数中的包围盒 */
    void extend(const BoundingBox &bbox)
    {
        this->maxPoint = vec3SelectMax(bbox.maxPoint, this->maxPoint);
        this->minPoint = vec3SelectMin(bbox.minPoint, this->minPoint);
        return;
    }

    /** @brief 该结点是否是叶子结点 */
    bool isLeaf() const
    {
        return left == nullptr && right == nullptr;
    }
};

/* KD-Tree 结构, 管理场景中的模型 */
class KDTree
{
public:
    KDTree() : root(nullptr) {}
    KDTree(const Model &model)
    {
        this->build(model);
    }
    ~KDTree() { this->release(); }

    void release()
    {
        if (root != nullptr)
        {
            root->release();
            root = nullptr;
        }
        return;
    }

    /** @brief 据 model 构造 KD-Tree */
    void build(const Model &model)
    {
        std::vector<size_t> faceIdces(model.faces.size());
        std::iota(faceIdces.begin(), faceIdces.end(), (size_t)0);
        this->root = this->buildRecursive(model, faceIdces, model.faces.size());
        return;
    }
    /** @brief 递归构造 KD-Tree */
    PKDNode buildRecursive(
        const Model &model,
        std::vector<size_t> &faceIdces,
        INT depth = 0);

public:
    PKDNode root; /* 根节点 */

    static constexpr INT DIMENSION = 3; /* 场景的维度 */
};

class KDTreeHierarchicalZBuffer : public HierarchicalZBuffer
{
public:
    KDTreeHierarchicalZBuffer() = delete;
    KDTreeHierarchicalZBuffer(const int windowsWidth, const int windowHeight)
        : HierarchicalZBuffer(windowsWidth, windowHeight)
    {
    }
    ~KDTreeHierarchicalZBuffer() = default;
    void init()
    {
        this->HierarchicalZBuffer::init();
        this->kdTree.release();
    }
    void renderModel(const Model &model) override;

private:
    KDTree kdTree;

    /** @brief 渲染 KD-Tree 结点中包含的物体 */
    void renderKDTreeNode(const Model &model, PKDNode pKDNode, PQuadNode pQuadNode);
};