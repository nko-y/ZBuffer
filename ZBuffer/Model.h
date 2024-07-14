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

	//���������ʹ���ܹ�ȫ����ʾ����Ļ����
	void ScaleToShow(int w, int h);

	//Ϊ��Ƭ������ɫ��Ϣ
	void SetFaceColor();
	vec3 lightPosition;
	vec3 lightColor;
	vec3 ambientColor;

	//��ת����
	void rotateFace(double theta, double n1, double n2, double n3);
	const int pi = 3.1415926535897;

	//ģ�͵İ˲���
	OCTree* octree;
	void BuildOCTree();
	void releaseOCTree();
	void relaseOCTNode(OCTNode *node);
};

