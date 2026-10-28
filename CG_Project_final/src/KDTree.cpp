#include "KDTree.h"

#include <algorithm>

KDNode::KDNode(PKDNode left, PKDNode right)
    : BoundingBox{
            SFINIT(.minPoint, POINT_MAX),
            SFINIT(.maxPoint, POINT_LOWEST)},
        left(left), right(right)
{
    ASSERT(!(left == nullptr && right == nullptr), "Invalid argument!");
    if (left != nullptr && right != nullptr)
    {
        /* Internal nodes only need bounds; keeping all descendant faces here
           turns a balanced tree into O(n log n) pointer storage. */
        this->extend(*this->left);
        this->extend(*this->right);
    }
    else if (left != nullptr && right == nullptr)
    {
        this->extend(*this->left);
    }
    else if (left == nullptr && right != nullptr)
    {
        this->extend(*this->right);
    }
    return;
}

void KDNode::release() {
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

bool KDNode::hit(Ray &ray, Intersection& intersection)
{
    bool ret = false;
    if (this->intersect(ray)) {
        if (this->isLeaf() /*&& !this->faceVec.empty() */) {
            ret |= this->faceVec.front()->hit(ray, intersection);
        }
        else {
            if (this->left != nullptr) {
                ret |= this->left->hit(ray, intersection);
            }
            if (this->right != nullptr) {
                ret |= this->right->hit(ray, intersection);
            }
        }
    }
    return ret;
}

void KDTree::release() {
    if (this->root != nullptr)
    {
        this->root->release();
        delete this->root;
        this->root = nullptr;
    }
    return;
}


PKDNode KDTree::buildRec(
    std::vector<Face *> &faceVec,
    const unsigned depth
)
{
    PKDNode node;
    if (faceVec.empty()) {
        node = nullptr;
    }
    else if (faceVec.size() == (size_t)1) {
        /* leaf node */
        /* 以单个面片的包围盒作为叶子结点。 */
        node = new KDNode(faceVec.front()->getBoundingBox());
        node->faceVec = faceVec;
    }
    else
    {
        /* 沿着 axis 轴进行分割
           每层结点轮换使用不同的轴 */
        const unsigned axis = depth % KDTree::dimension;
        const size_t mid = (faceVec.size() - 1) / 2;

        std::nth_element(
            faceVec.begin(), faceVec.begin() + mid, faceVec.end(),
            [&](const Face* i, const Face* j) -> bool
            {
                return i->getBoundingBox().center()[axis] <
                       j->getBoundingBox().center()[axis];
            });
        
        /* 左结点包含的面片 */
        std::vector<Face *> faceVecLeft(faceVec.begin(), faceVec.begin() + mid + 1);
        /* 右结点包含的面片 */
        std::vector<Face *> faceVecRight(faceVec.begin() + mid + 1, faceVec.end());

        node = new KDNode(
            this->buildRec(faceVecLeft, 1 + depth),
            this->buildRec(faceVecRight, 1 + depth));
    }

    return node;
}
