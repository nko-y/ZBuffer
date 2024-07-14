#pragma once

#include "Model.h"
#include <vector>
#include <math.h>
#include <algorithm>
#include <ctime>



class ScanLineZBuffer
{
public:
	//分类边表结构单元
	struct Edge
	{
		double x;
		double dx;
		int dy;
		int id;
	};

	//活化的边表结构单元
	struct ActiveEdge
	{
		double x;
		double dx;
		int dy;
		double z;
		double dzx;
		double dzy;
		int id;
	};

	//分类多边形表结构单元
	struct Polygon
	{
		double a, b, c, d;
		int id;
		int dy;
		std::vector<ActiveEdge> ActiveEdgeTable;
	};

public:
	ScanLineZBuffer(int W, int H);
	~ScanLineZBuffer();

	void ResizeZBuffer(int width, int height);
	void InitializeAllStructure(Model* m);
	void RenewAllBuffer(Model* m);
	bool EdgeCmp(const ActiveEdge& first, const ActiveEdge& second);

	void ZBufferUpdate(Model *m);

	//当前Z-Buffer是否需要更新
	bool ShouldRenew;
	//大小
	int SizeWidth, SizeHeight;

	//存储当前像素应该包含哪个多边形
	int** pixelId;
	double* oneLineDepth;
	std::vector<std::vector<Polygon>> PolygonTable;
	std::vector<std::vector<Edge>> EdgeTable;
	std::vector<Polygon> ActivePolygonTable;
};

