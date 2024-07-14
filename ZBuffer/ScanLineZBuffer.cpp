#include "ScanLineZBuffer.h"
#include <omp.h>

using namespace std;

ScanLineZBuffer::ScanLineZBuffer(int W, int H) {
	ShouldRenew = true;
	SizeWidth = 0;
	SizeHeight = 0;
	pixelId = NULL;
	oneLineDepth = NULL;
	ResizeZBuffer(W, H);
}


ScanLineZBuffer::~ScanLineZBuffer() {
	//Free all space
	if (pixelId != NULL) {
		for (int i = 0; i < SizeHeight; i++) {
			delete[] pixelId[i];
			pixelId[i] = NULL;
		}
		delete[] pixelId;
		pixelId = NULL;
	}
	if (oneLineDepth != NULL) {
		delete[] oneLineDepth;
		oneLineDepth = NULL;
	}
}


void ScanLineZBuffer::ResizeZBuffer(int width, int height) {
	if (SizeHeight == height && SizeWidth == width) return;

	//Free all space
	if (pixelId != NULL) {
		for (int i = 0; i < SizeHeight; i++) {
			delete[] pixelId[i];
			pixelId[i] = NULL;
		}
		delete[] pixelId;
		pixelId = NULL;
	}
	if (oneLineDepth != NULL) {
		delete[] oneLineDepth;
		oneLineDepth = NULL;
	}

	this->SizeWidth = width;
	this->SizeHeight = height;
	ShouldRenew = true;

	oneLineDepth = new double[SizeWidth];
	pixelId = new int* [SizeHeight];
	for (int i = 0; i < SizeHeight; i++) {
		pixelId[i] = new int[SizeWidth];
	}
	//cout << "ScanLineZBuffer:" << SizeHeight << " " << SizeWidth << endl;
	//cout << endl;
}



void ScanLineZBuffer::ZBufferUpdate(Model* m) {
	if (!ShouldRenew) return;

	clock_t t = clock();

	m->ScaleToShow(SizeWidth, SizeHeight);
	m->SetFaceColor();

	InitializeAllStructure(m);
	RenewAllBuffer(m);

	ShouldRenew = false;

	cout << "ScanLineZBuffer Update Time: " << double(clock() - t) << "ms" << endl;
	cout << endl;
}


void ScanLineZBuffer::InitializeAllStructure(Model* m) {
	PolygonTable.clear();
	PolygonTable.resize(SizeHeight);
	EdgeTable.clear();
	EdgeTable.resize(SizeHeight);
	ActivePolygonTable.clear();

	omp_lock_t lock;
	omp_init_lock(&lock);
#pragma omp parallel for
	for (int i = 0, fsize= m->allF.size(); i < fsize; i++) {
		Polygon pl;
		Face& fc = m->allF[i];
		pl.id = i;
		
		//����ÿһ����
		double minY, maxY;
		minY = INT_MAX;
		maxY = INT_MIN;
		for (int j = 0, vsize = fc.vertexIndex.size(); j < vsize; j++) {
			vec3 nowPoint = m->allV[fc.vertexIndex[j]].pos;
			int nextj = (j == vsize - 1 ? 0 : j + 1);
			vec3 nextPoint = m->allV[fc.vertexIndex[nextj]].pos;
			//���ϸߵĵ����nowPoint
			if (nowPoint[1] < nextPoint[1]) {
				vec3 tempPoint = nowPoint;
				nowPoint = nextPoint;
				nextPoint = tempPoint;
			}
			//�洢Ϊһ����
			Edge eg;
			eg.dy = round(nowPoint.y()) - round(nextPoint.y());
			if (eg.dy <= 0) continue;
			eg.x = nowPoint.x();
			eg.dx = -(nowPoint.x() - nextPoint.x()) / (nowPoint.y() - nextPoint.y());
			eg.id = i;
			
			//����߱�
			omp_set_lock(&lock);
			EdgeTable[round(nowPoint.y())].push_back(eg);
			omp_unset_lock(&lock);

			//�������Сֵ�Ƚ�
			if (nowPoint.y() > maxY) maxY = nowPoint.y();
			if (nextPoint.y() < minY) minY = nextPoint.y();
		}

		pl.dy = round(maxY) - round(minY);
		if (pl.dy <= 0 || minY >= SizeHeight || maxY <= 0) {
			continue;
		}
		pl.a = fc.norm.x();
		pl.b = fc.norm.y();
		pl.c = fc.norm.z();
		vec3& onePoint = m->allV[fc.vertexIndex[0]].pos;
		pl.d = -(pl.a * onePoint.x() + pl.b * onePoint.y() + pl.c * onePoint.z());
		//��ӽ������α�
		omp_set_lock(&lock);
		PolygonTable[round(maxY)].push_back(pl);
		omp_unset_lock(&lock);
	}
	omp_destroy_lock(&lock);

	//int cntEdge = 0, cntPoly = 0;
	//for (int i = 0; i < SizeHeight; i++) {
	//	cntEdge += EdgeTable[i].size();
	//	cntPoly += PolygonTable[i].size();
	//}
}


bool ScanLineZBuffer::EdgeCmp(const ActiveEdge& first, const ActiveEdge& second) {
	if (round(first.x) < round(second.x)) return true;
	//�Ƿ���ҪҲ��round
	else if (round(first.x) == round(second.x) && first.dx <= second.dx) return true;
	else return false;
}


void ScanLineZBuffer::RenewAllBuffer(Model* m) {
	for (int y = SizeHeight - 1; y >= 0; y--) {
		//���Ƚ����е�Ԫ�ع���
		memset(pixelId[y], -1, sizeof(int) * SizeWidth);
		for (int i = 0; i < SizeWidth; i++) {
			oneLineDepth[i] = INT_MIN;
		}

		//������µĶ����������Ķ���α�
		for (auto& i : PolygonTable[y]) {
			ActivePolygonTable.push_back(i);
		}

#pragma omp parallel for
		for (int i = 0, plsize = ActivePolygonTable.size(); i < plsize; i++) {
			//����ÿ�������Σ��Ѷ�Ӧ�ı߼����߱�
			Polygon& selectPolyon = ActivePolygonTable[i];
			for (auto& oneEdge : EdgeTable[y]) {
				if (oneEdge.id != selectPolyon.id) continue;
				ActiveEdge tempActiveEdge;
				tempActiveEdge.x = oneEdge.x;
				tempActiveEdge.dx = oneEdge.dx;
				tempActiveEdge.dy = oneEdge.dy;
				if (fabs(selectPolyon.c) < 0.000001) {
					tempActiveEdge.z = 0;
					tempActiveEdge.dzx = 0;
					tempActiveEdge.dzy = 0;
				}
				else
				{
					tempActiveEdge.z = -(selectPolyon.a * oneEdge.x + selectPolyon.b * y + selectPolyon.d) / selectPolyon.c;
					tempActiveEdge.dzx = -(selectPolyon.a / selectPolyon.c);
					tempActiveEdge.dzy = selectPolyon.b / selectPolyon.c;
				}
				selectPolyon.ActiveEdgeTable.push_back(tempActiveEdge);
				oneEdge.id = -1;
			}
			//����Щ�߽�������
			if (selectPolyon.id == 215) {
				int a = 1;
			}
			for (int m = selectPolyon.ActiveEdgeTable.size() - 1; m > 0; m--) {
				for (int n = 0; n < m; n++) {
					if (!EdgeCmp(selectPolyon.ActiveEdgeTable[n], selectPolyon.ActiveEdgeTable[n + 1])) {
						ActiveEdge tp = selectPolyon.ActiveEdgeTable[n];
						selectPolyon.ActiveEdgeTable[n] = selectPolyon.ActiveEdgeTable[n + 1];
						selectPolyon.ActiveEdgeTable[n + 1] = tp;
					}
				}
			}
			//���δ���߶�
			for (auto firstActiveEdge = selectPolyon.ActiveEdgeTable.begin(); firstActiveEdge != selectPolyon.ActiveEdgeTable.end(); firstActiveEdge++) {
				ActiveEdge& first = *firstActiveEdge;
				ActiveEdge& second = *(++firstActiveEdge);
				double nowz = first.z;
				//���ﲻ��Ҫȡ���ں���
				for (int x = round(first.x), endx = round(second.x); x < endx; x++) {
					if (nowz > oneLineDepth[x]) {
						oneLineDepth[x] = nowz;
						pixelId[y][x] = selectPolyon.id;
					}
					nowz += first.dzx;
				}
				first.dy--;
				second.dy--;
				first.x += first.dx;
				second.x += second.dx;
				first.z += first.dzx * first.dx + first.dzy;
				second.z += second.dzx * second.dx + second.dzy;
			}

			//ȥ���Ѿ������˵ı�
			int record = 0;
			for (int cnt = 0, endcnt = selectPolyon.ActiveEdgeTable.size(); cnt < endcnt; cnt++) {
				if (selectPolyon.ActiveEdgeTable[cnt].dy <= 0) continue;
				selectPolyon.ActiveEdgeTable[record] = selectPolyon.ActiveEdgeTable[cnt];
				record++;
			}
			selectPolyon.ActiveEdgeTable.resize(record);
			//��Ӧ�Ķ����Ҳ��һ
			selectPolyon.dy--;
		}
		//ȥ���Ѿ������˵Ķ����
		int record = 0;
		for (int cnt = 0, endcnt = ActivePolygonTable.size(); cnt < endcnt; cnt++) {
			if (ActivePolygonTable[cnt].dy <= 0) continue;
			ActivePolygonTable[record] = ActivePolygonTable[cnt];
			record++;
		}
		ActivePolygonTable.resize(record);
	}
}