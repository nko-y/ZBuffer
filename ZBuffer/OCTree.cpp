#include "OCTree.h"

void OCTNode::splitNode(int maxTriNum){
	Bound allBound[8];
	vec3 &min_p = b.min_point;
	vec3& max_p = b.max_point;
    vec3 middle_p = (min_p + max_p) / 2;

    //- - +
    allBound[0] = { min_p[0], min_p[1], middle_p[2], middle_p[0], middle_p[1], max_p[2] };
    //+ - +
    allBound[1] = { middle_p[0], min_p[1], middle_p[2], max_p[0], middle_p[1], max_p[2] };
    //- + +
    allBound[2] = { min_p[0], middle_p[1], middle_p[2], middle_p[0], max_p[1], max_p[2] };
    //+ + +
    allBound[3] = { middle_p[0], middle_p[1], middle_p[2], max_p[0], max_p[1], max_p[2] };
    //- - -
    allBound[4] = { min_p[0], min_p[1], min_p[2], middle_p[0], middle_p[1], middle_p[2] };
    //+ - -
    allBound[5] = { middle_p[0], min_p[1], min_p[2], max_p[0], middle_p[1], middle_p[2] };
    //- + -
    allBound[6] = { min_p[0], middle_p[1], min_p[2], middle_p[0], max_p[1], middle_p[2] };
    //+ + -
    allBound[7] = { middle_p[0], middle_p[1], min_p[2], max_p[0], max_p[1], middle_p[2] };

#pragma omp parallel for
    //¹¹½¨OCTree
    for (int i = 0; i < 8; i++) {
        child[i] = new OCTNode(allBound[i]);
        for (int j = 0, tsize = allTri.size(); j < tsize; j++) {
            if (child[i]->b.isIntersectTri(*allTri[j])) {
                child[i]->addTri(allTri[j]);
            }
        }
    }
    //µÝ¹éÆÊ·Ö
    this->allTri.clear();
    for (int i = 0; i < 8; i++) {
        if (child[i] != NULL && child[i]->allTri.size()>maxTriNum) {
            this->child[i]->splitNode(maxTriNum);
        }
    }
}

void OCTree::buildTree() {
    int tsize = allTri.size();
    if (tsize <= 0) return;

    Bound spaceBound;
    for (int i = 0; i < tsize; i++) {
        spaceBound.union_triangle(*allTri[i]);
    }
    root = new OCTNode(spaceBound);
    for (int i = 0; i < tsize; i++) {
        root->addTri(allTri[i]);
    }
    root->splitNode(maxTriNum);
}