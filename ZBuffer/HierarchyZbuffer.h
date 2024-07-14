#pragma once

#include "Model.h"
#include <math.h>
#include <iostream>
#include <ctime>
#include "vec3.h"

class HierarchyZbuffer
{
public:
	//特殊的活化的边表,用于单个三角形的扫描转换
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

	//当前Z-Buffer是否需要更新
	bool ShouldRenew;
	//大小
	int SizeWidth, SizeHeight;

	//存储当前像素应该包含哪个多边形
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

	//用于下次更新zbuffer时作为时间上的先验
	bool* LastFrameUsed;
	bool NeedRecord = true;
};

