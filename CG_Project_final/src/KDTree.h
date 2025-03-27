#pragma once

#include "Geometry.h"

#include <functional>

struct KDNode;
typedef KDNode *PKDNode;


/*  KD-Tree 结点; 
    结点本身是一个包围盒, 其中记录了包围盒中包含的面片;
    左子树和右子树是对该包围盒进行分割得到的更小的包围盒。 */
struct KDNode : BoundingBox
{
public:
    /* 构建叶子结点 */
    KDNode(const BoundingBox &bbox)
        : BoundingBox(bbox),
          left(nullptr),
          right(nullptr)
    {}

    /* 构建非叶子结点 */
    KDNode(PKDNode left, PKDNode  right);

    /* 递归释放子结点 */
    void release();
    /* 是否是叶子结点 */
    bool isLeaf() const
    {
        return left == nullptr && right == nullptr;
    }

    bool hit(Ray &ray, Intersection& intersection);

    PKDNode  left;  /* 左子树 */
    PKDNode  right; /* 右子树 */
    std::vector<Face *> faceVec; /* 该结点中包含的面片 */ 
};

/* KD-Tree: 
    https://en.wikipedia.org/wiki/K-d_tree */
/* 管理模型的所有面片, 用于加速模型和射线求交。 */
/* 由于存放的是 Face * 指针, 因此模型的 faces 变化后必需重新构建 KD-Tree。 */
class KDTree {
public:
    KDTree() : root(nullptr) {}
    KDTree(std::vector<Face *>& faceVec) {
        this->build(faceVec);
    }
    ~KDTree() { this->release(); }
    void release();

    bool empty() const { return this->root == nullptr; }

    void build(std::vector<Face *>& faceVec) { this->root = this->buildRec(faceVec); }
    PKDNode buildRec(std::vector<Face *>& faceVec, const unsigned depth = 0);

public:
    /* 空间维数 */
    static constexpr unsigned dimension = 3;
    /* KD-Tree 的根节点 (即包围整个模型的包围盒) */
    PKDNode root;

};