#pragma once

#include "Model.h"
#include <vector>
#include <math.h>
#include <algorithm>
#include <ctime>



class ScanLineZBuffer
{
public:
	//����߱�ṹ��Ԫ
	struct Edge
	{
		double x;
		double dx;
		int dy;
		int id;
	};

	//��ı߱�ṹ��Ԫ
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

	//�������α�ṹ��Ԫ
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

	//��ǰZ-Buffer�Ƿ���Ҫ����
	bool ShouldRenew;
	//��С
	int SizeWidth, SizeHeight;

	//�洢��ǰ����Ӧ�ð����ĸ������
	int** pixelId;
	double* oneLineDepth;
	std::vector<std::vector<Polygon>> PolygonTable;
	std::vector<std::vector<Edge>> EdgeTable;
	std::vector<Polygon> ActivePolygonTable;
};

