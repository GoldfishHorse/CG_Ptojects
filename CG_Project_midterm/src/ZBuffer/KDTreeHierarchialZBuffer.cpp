#include "KDTreeHierarchialZBuffer.h"

#include <algorithm>

/* 
    递归为场景面片构建 KDTree,
    每次轮换地选择一层坐标轴对空间进行切割。
    思路参考了 https://github.com/gishi523/kd-tree/blob/master/kdtree.h#L156
 */
PKDNode KDTree::buildRecursive(
    const Model &model,
    std::vector<size_t> &faceIdces,
    INT depth)
{
    // LOG_DEBUG("nfaces = %zu.", faceIdces.size());
    PKDNode node;
    if (faceIdces.empty())
    {
        node = nullptr;
    }
    else if (faceIdces.size() == (size_t)1)
    {
        /* leaf node */
        /* 以单个面片的包围盒作为叶子结点。 */
        node = new KDNode(model.bboxFace.at(faceIdces.front()));
        node->faceIdces = std::move(faceIdces);
    }
    else
    {
        // LOG_DEBUG("Split Node.");
        /* 沿着 axis 轴进行分割
           每层结点轮换使用不同的轴 */
        const INT axis = depth % KDTree::DIMENSION;
        const size_t mid = (faceIdces.size() - 1) / 2;

        std::nth_element(
            faceIdces.begin(), faceIdces.begin() + mid, faceIdces.end(),
            [&](const size_t i, const size_t j) -> bool
            {
                /* D3DXVECTOR3 可以转换为指向其结构体的指针 */
                return ((const FLOAT *)model.bboxFace.at(i).maxPoint)[axis] <
                       ((const FLOAT *)model.bboxFace.at(j).maxPoint)[axis];
            });
        
        /* 左结点包含的面片 */
        std::vector<size_t> faceIdcesLeft(faceIdces.begin(), faceIdces.begin() + mid + 1);
        /* 右结点包含的面片 */
        std::vector<size_t> faceIdcesRight(faceIdces.begin() + mid + 1, faceIdces.end());

        node = new KDNode(
            this->buildRecursive(model, faceIdcesLeft, 1 + depth),
            this->buildRecursive(model, faceIdcesRight, 1 + depth));
        node->faceIdces = std::move(faceIdces);
    }

    return node;
}

void KDTreeHierarchicalZBuffer::renderModel(const Model &model)
{
    LOG_INFO("KDTree Hierarchical ZBuffer rendering...");
    this->init();
    this->kdTree.build(model);
    this->renderKDTreeNode(model, this->kdTree.root, this->zPyramid.root);
    return;
}

void KDTreeHierarchicalZBuffer::renderKDTreeNode(const Model &model, PKDNode pKDNode, PQuadNode pQuadNode)
{
    if (pKDNode->maxPoint.z <= pQuadNode->z)
    {
        return;
    }

    /*  KD-Tree 到达了叶子结点: 只包含一个面片的结点。
        直接渲染该面片。 */
    if (pKDNode->isLeaf())
    {
        // LOG_DEBUG("Render face %zu.", pKDNode->faceIdces.front());
        this->HierarchicalZBuffer::renderFace(pKDNode->faceIdces.front(), model, pQuadNode);
        return;
    }

    /* 四叉树存在一个子结点包含了 KD-Tree 结点的 (包围盒的) 投影。
     使用四叉树的子结点来渲染。*/
    if (
        auto it = std::find_if(pQuadNode->children.cbegin(), pQuadNode->children.cend(),
                               [&pKDNode](const PQuadNode pChildQuadNode) -> bool
                               {
                                   return pChildQuadNode->includes(*pKDNode);
                               });
        it != pQuadNode->children.cend())
    {
        renderKDTreeNode(model, pKDNode, *it);
    }
    /*  四叉树结点的子结点都不能包含 KD-Tree 结点的 (包围盒的) 投影;
        对 KD-Tree 的左右子树进行渲染。 */
    else
    {
        if (pKDNode->left != nullptr)
        {
            renderKDTreeNode(model, pKDNode->left, pQuadNode);
        }
        if (pKDNode->right != nullptr)
        {
            renderKDTreeNode(model, pKDNode->right, pQuadNode);
        }
    }
    return;
}