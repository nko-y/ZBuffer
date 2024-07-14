#pragma once

#include <iostream>
#include <string.h>
#include <fstream>
#include <vector>
#include <climits>
#include "vec3.h"
#include "OCTree.h"

class Model
{
public:
	Model(std::string Path);
	~Model();

	std::vector<Vertex> allV;
	std::vector<Face> allF;
	std::vector<vec3> allN;
	std::vector<Triangle> allTri;
	vec3 modelCenter;

	//把物体放缩使得能够全部显示在屏幕当中
	void ScaleToShow(int w, int h);

	//为面片赋予颜色信息
	void SetFaceColor();
	vec3 lightPosition;
	vec3 lightColor;
	vec3 ambientColor;

	//旋转物体
	void rotateFace(double theta, double n1, double n2, double n3);
	const int pi = 3.1415926535897;

	//模型的八叉树
	OCTree* octree;
	void BuildOCTree();
	void releaseOCTree();
	void relaseOCTNode(OCTNode *node);
};

