#include "HierarchyZbuffer.h"

using namespace std;

HierarchyZbuffer::HierarchyZbuffer(int W, int H) {
	ShouldRenew = true;
	SizeWidth = 0;
	SizeHeight = 0;
	pixelId = NULL;
	hdepth = NULL;
	LastFrameUsed = NULL;
	level = 0;
	ResizeZBuffer(W, H);
}


HierarchyZbuffer::~HierarchyZbuffer() {
	FreeAllSpace();
	delete[] LastFrameUsed;
}

void HierarchyZbuffer::FreeAllSpace() {
	if (pixelId != NULL) {
		for (int i = 0; i < SizeHeight; i++) {
			delete[] pixelId[i];
			pixelId[i] = NULL;
		}
		delete[] pixelId;
		pixelId = NULL;
	}
	if (hdepth != NULL) {
		for (int i = 0; i < levelAttribute.size(); i++) {
			for (int j = 0; j < levelAttribute[i].maxH; j++) {
				delete[] hdepth[i][j];
				hdepth[i][j] = NULL;
			}
			delete[] hdepth[i];
			hdepth[i] = NULL;
		}
		delete[] hdepth;
		hdepth = NULL;
	}
	levelAttribute.clear();
}

void HierarchyZbuffer::ResizeZBuffer(int width, int height) {
	if (SizeHeight == height && SizeWidth == width) return;

	FreeAllSpace();

	this->SizeWidth = width;
	this->SizeHeight = height;
	ShouldRenew = true;

	pixelId = new int* [SizeHeight];
	for (int i = 0; i < SizeHeight; i++) {
		pixelId[i] = new int[SizeWidth];
	}
	//分配层次zbuffer的空间
	level = 1;
	int cnt = 1;
	while (true) {
		Attri tp;
		tp.levelNum = cnt;
		levelAttribute.push_back(tp);
		if (width / cnt == 0 || height / cnt == 0) break;
		level++;
		cnt = cnt * 2;
	}
	hdepth = new double** [level];
	for (int i = 0; i < level; i++) {
		int levelHeight = SizeHeight / levelAttribute[i].levelNum;
		if (levelAttribute[i].levelNum * levelHeight < SizeHeight) levelHeight++;
		int levelWidth = SizeWidth / levelAttribute[i].levelNum;
		if (levelAttribute[i].levelNum * levelWidth < SizeWidth) levelWidth++;
		levelAttribute[i].maxH = levelHeight;
		levelAttribute[i].maxW = levelWidth;
		hdepth[i] = new double* [levelHeight];
		for (int j = 0; j < levelHeight; j++) {
			hdepth[i][j] = new double[levelWidth];
		}
	}
	//for (int i = 0; i < level; i++) {
	//	cout << levelAttribute[i].levelNum << " " << levelAttribute[i].maxH << " " << levelAttribute[i].maxW << endl;
	//}
	//cout << "HierarchyZBuffer:" << SizeHeight << " " << SizeWidth << endl;
	//cout << "HierarchyZBuffer Level Number:" << level << endl;
	//cout << endl;
}


void HierarchyZbuffer::ZBufferUpdate(Model* m) {
	if (!ShouldRenew) return;

	clock_t t = clock();

	m->ScaleToShow(SizeWidth, SizeHeight);
	m->SetFaceColor();

	InitializeAllStructure(m);
	RenewAllBuffer(m);

	ShouldRenew = false;

	cout << "HierarchyZBuffer Update Time: " << double(clock() - t) << "ms" << endl;
	cout << endl;
}

void HierarchyZbuffer::updateAreaDepth(int fromx, int tox, int fromy, int toy) {
	int lastLevelMaxX, lastLevelMaxY, lastLevel;
	for (int i = 1; i < level; i++) {
		fromx = fromx >> 1; tox = tox >> 1;
		fromy = fromy >> 1; toy = toy >> 1;
		lastLevelMaxX = levelAttribute[i - 1].maxW;
		lastLevelMaxY = levelAttribute[i - 1].maxH;
		lastLevel = i - 1;
#pragma omp parallel for
		for (int m = fromx; m <= tox; m++) {
			for (int n = fromy; n <= toy; n++) {
				double Num;
				double minNum=INT_MAX;
				int startx = m << 1;
				int starty = n << 1;
				//左下
				Num = hdepth[lastLevel][starty][startx];
				if (Num < minNum) minNum = Num;
				//右下
				if (startx + 1 < lastLevelMaxX) {
					Num = hdepth[lastLevel][starty][startx + 1];
					if (Num < minNum) minNum = Num;
				}
				//左上
				if (starty + 1 < lastLevelMaxY) {
					Num = hdepth[lastLevel][starty+1][startx];
					if (Num < minNum) minNum = Num;
				}
				//右上
				if (startx + 1 < lastLevelMaxX && starty + 1 < lastLevelMaxY) {
					Num = hdepth[lastLevel][starty + 1][startx + 1];
					if (Num < minNum) minNum = Num;
				}
				hdepth[i][n][m] = minNum;
			}
		}
	}
}

void HierarchyZbuffer::InitializeAllStructure(Model* m) {
	//把所有多边形指向归为-1
	for (int y = 0; y < SizeHeight - 1; y++) {
		memset(pixelId[y], -1, sizeof(int) * SizeWidth);
	}
	//把所有的深度归为最小值
	for (int i = 0; i < levelAttribute[0].maxH; i++) {
		fill(hdepth[0][i], hdepth[0][i] + levelAttribute[0].maxW, INT_MIN);
	}
	//向上更新
	updateAreaDepth(0, SizeWidth - 1, 0, SizeHeight - 1);
}


bool HierarchyZbuffer::judgeCulling(int depthlevel, double thisDepth, int fromx, int fromy, int tox, int toy) {
	for (int i = fromx; i <=tox; i++) {
		for (int j = fromy; j <= toy; j++) {
			if (thisDepth > hdepth[depthlevel][j][i]) return false;
		}
	}
	return true;
}

bool HierarchyZbuffer::EdgeCmp(const ActiveEdge& first, const ActiveEdge& second) {
	if (round(first.x) < round(second.x)) return true;
	//是否需要也用round
	else if (round(first.x) == round(second.x) && first.dx <= second.dx) return true;
	else return false;
}


void HierarchyZbuffer::RenewAllBuffer(Model* m) {
	int fsize = m->allF.size();

	//是否先绘制上一帧已经绘制过的图形
	vector<int> faceidSquence;
	if (NeedRecord && LastFrameUsed!=NULL) {
		for (int i = 0; i < fsize; i++) {
			if (LastFrameUsed[i]) faceidSquence.push_back(i);
		}
		for (int i = 0; i < fsize; i++) {
			if (!LastFrameUsed[i]) faceidSquence.push_back(i);
		}
	}
	else
	{
		for (int i = 0; i < fsize; i++) {
			faceidSquence.push_back(i);
		}
	}
	
	//遍历每一个三角形，将其存入zbuffer
	for (auto &seqFace : faceidSquence) {
		int iface = seqFace;
		//进行背面剔除
		//if (m->allF[iface].norm.z() < 0) continue;

		Face& f = m->allF[iface];
		vec3& v0 = m->allV[f.vertexIndex[0]].pos;
		vec3& v1 = m->allV[f.vertexIndex[1]].pos;
		vec3& v2 = m->allV[f.vertexIndex[2]].pos;

		//得出最大最小
		double minx, maxx, miny, maxy, minz, maxz;
		int minxi, maxxi, minyi, maxyi;
		minx = maxx = v0.x();
		miny = maxy = v0.y();
		minz = maxz = v0.z();

		if (v1.x() < minx) minx = v1.x();
		if (v2.x() < minx) minx = v2.x();

		if (v1.x() > maxx) maxx = v1.x();
		if (v2.x() > maxx) maxx = v2.x();

		if (v1.y() < miny) miny = v1.y();
		if (v2.y() < miny) miny = v2.y();

		if (v1.y() > maxy) maxy = v1.y();
		if (v2.y() > maxy) maxy = v2.y();

		if (v1.z() < minz) minz = v1.z();
		if (v2.z() < minz) minz = v2.z();

		if (v1.z() > maxz) maxz = v1.z();
		if (v2.z() > maxz) maxz = v2.z();

		//判断是否被遮挡
		maxxi = round(maxx);
		minxi = round(minx);
		maxyi = round(maxy);
		minyi = round(miny);
		int xLen = maxxi - minxi;
		int yLen = maxyi - minyi;
		int maxL = max(xLen, yLen);
		int levelEst = ceil(log2(max(maxL, 1)));
		if (levelEst >= level) levelEst = level - 1;
		bool judge = judgeCulling(levelEst, maxz, minxi >> levelEst, minyi >> levelEst, maxxi >> levelEst, maxyi >> levelEst);
		if (judge) continue;
		
		//没有被遮挡则需要更新这个多边形对应的zbuffer
		double func_a = f.norm.x();
		double func_b = f.norm.y();
		double func_c = f.norm.z();
		double func_d = -(func_a * v0.x() + func_b * v0.y() + func_c * v0.z());
		double dzx = 0.0;
		double dzy = 0.0;
		if (fabs(func_c) >= 0.000001) {
			dzx = -(func_a / func_c);
			dzy = func_b / func_c;
		}
		vector<ActiveEdge> AllEdge;
		vector<ActiveEdge> ActiveEdgeTable;
		for (int i = 0; i < 3; i++) {
			vec3 nowPoint = m->allV[f.vertexIndex[i]].pos;
			int nexti = (i == 2 ? 0 : i + 1);
			vec3 nextPoint = m->allV[f.vertexIndex[nexti]].pos;
			if (nowPoint[1] < nextPoint[1]) {
				vec3 tempPoint = nowPoint;
				nowPoint = nextPoint;
				nextPoint = tempPoint;
			}
			ActiveEdge eg;
			eg.dy = round(nowPoint.y()) - round(nextPoint.y());
			if (eg.dy <= 0) continue;
			eg.x = nowPoint.x();
			eg.dx = -(nowPoint.x() - nextPoint.x()) / (nowPoint.y() - nextPoint.y());
			eg.z = nowPoint.z();
			eg.y = round(nowPoint.y());
			AllEdge.push_back(eg);
		}
		for (int y = maxyi; y >= minyi; y--) {
			int last = 0;
			bool change = false;
			for (int i = 0, end = AllEdge.size(); i < end; i++) {
				if (AllEdge[i].y == y) {
					ActiveEdgeTable.push_back(AllEdge[i]);
					change = true;
				}
			}
			if (change && !EdgeCmp(ActiveEdgeTable[0], ActiveEdgeTable[1])) {
				ActiveEdge temp = ActiveEdgeTable[0];
				ActiveEdgeTable[0] = ActiveEdgeTable[1];
				ActiveEdgeTable[1] = temp;
			}
			if (ActiveEdgeTable.size() == 0) continue;
			ActiveEdge& first = ActiveEdgeTable[0];
			ActiveEdge& second = ActiveEdgeTable[1];
			double nowz = first.z;
			for (int x = round(first.x), endx = round(second.x); x < endx; x++) {
				if (nowz > hdepth[0][y][x]) {
					hdepth[0][y][x] = nowz;
					pixelId[y][x] = iface;
				}
				nowz += dzx;
			}
			first.dy--;
			second.dy--;
			first.x += first.dx;
			second.x += second.dx;
			first.z += dzx * first.dx + dzy;
			second.z += dzx * second.dx + dzy;
			//去掉已经结束的边
			int record = 0;
			for (int cnt = 0, endcnt = ActiveEdgeTable.size(); cnt < endcnt; cnt++) {
				if (ActiveEdgeTable[cnt].dy <= 0) continue;
				ActiveEdgeTable[record] = ActiveEdgeTable[cnt];
				record++;
			}
			ActiveEdgeTable.resize(record);
		}
		updateAreaDepth(minxi, maxxi, minyi, maxyi);
	}

	//如果需要，则记录上一帧用到了哪些多边形
	if (NeedRecord) {
		if (LastFrameUsed != NULL) {
			delete[] LastFrameUsed;
		}
		LastFrameUsed = new bool[m->allF.size()];
		for (int i = 0, msize = m->allF.size(); i < msize; i++) {
			LastFrameUsed[i] = false;
		}
		for (int i = 0; i < SizeHeight; i++) {
			for (int j = 0; j < SizeWidth; j++) {
				if (pixelId[i][j] >= 0) {
					LastFrameUsed[pixelId[i][j]] = true;
				}
			}
		}
	}
}