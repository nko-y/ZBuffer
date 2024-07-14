#pragma once

#include "Model.h"
#include <math.h>
#include <iostream>
#include <ctime>
#include "vec3.h"

class HierarchyZbuffer
{
public:
	//����Ļ�ı߱�,���ڵ��������ε�ɨ��ת��
	struct ActiveEdge
	{
		double x;
		int y;
		double dx;
		int dy;
		double z;
	};
public:
	HierarchyZbuffer(int W, int H);
	~HierarchyZbuffer();

	void FreeAllSpace();
	void ResizeZBuffer(int width, int height);
	void InitializeAllStructure(Model* m);
	void RenewAllBuffer(Model* m);
	void updateAreaDepth(int fromx, int tox, int fromy, int toy);
	bool judgeCulling(int depthlevel, double thisDepth, int fromx, int fromy, int tox, int toy);
	bool EdgeCmp(const ActiveEdge& first, const ActiveEdge& second);

	void ZBufferUpdate(Model* m);

	//��ǰZ-Buffer�Ƿ���Ҫ����
	bool ShouldRenew;
	//��С
	int SizeWidth, SizeHeight;

	//�洢��ǰ����Ӧ�ð����ĸ������
	int** pixelId;
	double*** hdepth;
	int level;
	struct Attri
	{
		int maxW;
		int maxH;
		int levelNum;
	};
	std::vector<Attri> levelAttribute;

	//�����´θ���zbufferʱ��Ϊʱ���ϵ�����
	bool* LastFrameUsed;
	bool NeedRecord = true;
};

