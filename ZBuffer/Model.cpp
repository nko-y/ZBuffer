#include "Model.h"
using namespace std;

Model::Model(string Path) {
	lightPosition = vec3(600.0f, 600.0f, 600.0f);
	lightColor = vec3(0.8, 0.8, 0.8);
	ambientColor = vec3(0.2, 0.2, 0.2);

	octree = NULL;

	ifstream fp(Path);
	if (!fp.is_open()) cout << "Failure to Load Model" << endl;

	string elem;
	char ch;
	vec3 point1, point2, point3;
	while (fp>>elem)
	{
		if (elem == "v") {
			Vertex tempVertex;
			fp >> tempVertex.pos[0] >> tempVertex.pos[1] >> tempVertex.pos[2];
			allV.push_back(tempVertex);
		}
		else if (elem == "vn") {
			vec3 tempNorm;
			fp >> tempNorm[0] >> tempNorm[1] >> tempNorm[2];
			allN.push_back(tempNorm);
		}
		else if (elem == "f") {
			Face tempFace;
			int vid, tid, nid;
			while (true) {
				//读取第一个vid
				ch = fp.get();
				if (ch == '\n' || ch == EOF) break;
				else if (ch == ' ') continue;
				else fp.putback(ch);
				fp >> vid;
				tempFace.vertexIndex.push_back(vid - 1);
				//读取第二个tid
				ch = fp.get();
				if (ch != '/') {
					fp.putback(ch);
					continue;
				}
				ch = fp.get();
				fp.putback(ch);
				if (ch != '/') {
					fp >> tid;
				}
				//读取第三个nid
				ch = fp.get();
				if (ch != '/') {
					fp.putback(ch);
					continue;
				}
				ch = fp.get();
				fp.putback(ch);
				if (ch >= '0' && ch<='9') {
					fp >> nid;
				}
				tempFace.normalIndex.push_back(nid - 1);
			}
			if (tempFace.vertexIndex.size() >= 2) {
				point1 = allV[tempFace.vertexIndex[0]].pos;
				point2 = allV[tempFace.vertexIndex[1]].pos;
				point3 = allV[tempFace.vertexIndex[2]].pos;
				tempFace.norm = cross(point2 - point1, point3 - point2).norm();
				tempFace.color = vec3(0.0, 0.0, 0.0);
				allF.push_back(tempFace);
			}
		}
	}

	//存下所有的三角形面片
	for (int i = 0, fsize = allF.size(); i < fsize; i++) {
		Triangle t;
		t.id = i;
		t.v0 = allV[allF[i].vertexIndex[0]].pos;
		t.v1 = allV[allF[i].vertexIndex[1]].pos;
		t.v2 = allV[allF[i].vertexIndex[2]].pos;
		allTri.push_back(t);
	}
	cout << "Successfully to Load Model" << endl;
	cout << "Vertex Number: " << allV.size() << endl;
	cout << "Face Number: " << allF.size() << endl;
	cout << endl;
}


Model::~Model() {
	releaseOCTree();
}


void Model::ScaleToShow(int w, int h) {
	double minX, minY, minZ, maxX, maxY, maxZ;
	minX = minY = minZ = INT_MAX;
	maxX = maxY = maxZ = INT_MIN;
	for (auto &i : allV) {
		if (i.pos[0] < minX) minX = i.pos[0];
		if (i.pos[1] < minY) minY = i.pos[1];
		if (i.pos[2] < minZ) minZ = i.pos[2];
		if (i.pos[0] > maxX) maxX = i.pos[0];
		if (i.pos[1] > maxY) maxY = i.pos[1];
		if (i.pos[2] > maxZ) maxZ = i.pos[2];
	}
	vec3 center = (vec3(minX, minY, minZ) + vec3(maxX, maxY, maxZ)) / 2.0;
	double xwidth = maxX - minX;
	double ywidth = maxY - minY;
	double xscale = w / xwidth;
	double yscale = h / ywidth;
	double needscale = xscale > yscale ? yscale : xscale;
	needscale = 0.8 * needscale;

	w /= 2.0;
	h /= 2.0;

#pragma omp parallel for
	for (auto& i : allV) {
		i.pos = (i.pos - center) * needscale;
		i.pos[0] += w;
		i.pos[1] += h;
	}
	modelCenter = vec3(xwidth, ywidth, 0.0);

	//cout << "now center is:" << w << "," << h << "," << "0.0" << endl;
	//cout << "scale size is:" << needscale << endl;
	//cout << endl;
}


void Model::SetFaceColor() {
#pragma omp parallel for
	for (auto& i : allF) {
		i.color = vec3(0.0, 0.0, 0.0);
		for (int j = 0; j < i.vertexIndex.size(); j++) {
			vec3 p = allV[i.vertexIndex[j]].pos;
			vec3 n = i.norm;
			if (i.normalIndex.size() > 0) n = allN[i.vertexIndex[j]].norm();
			vec3 ray = (lightPosition - p).norm();
			double I = dot(ray, n);
			if (I <= 0.0) I = 0.0;
			i.color = i.color + lightColor*I + ambientColor;
		}
		i.color = i.color / i.vertexIndex.size();
	}
}


void Model::rotateFace(double theta, double n1, double n2, double n3) {
	double sine = sin(theta / 180.0 * pi);
	double cosine = cos(theta / 180.0 * pi);
	double R[3][3];

	R[0][0] = n1 * n1 + (1 - n1 * n1) * cosine;
	R[0][1] = n1 * n2 * (1 - cosine) + n3 * sine;
	R[0][2] = n1 * n3 * (1 - cosine) - n2 * sine;
	R[1][0] = n1 * n2 * (1 - cosine) - n3 * sine;
	R[1][1] = n2 * n2 * (1 - cosine) + cosine;
	R[1][2] = n2 * n3 * (1 - cosine) + n1 * sine;
	R[2][0] = n1 * n2 * (1 - cosine) + n2 * sine;
	R[2][1] = n2 * n3 * (1 - cosine) - n1 * sine;
	R[2][2] = n3 * n3 + (1 - n3 * n3) * cosine;

#pragma omp parallel for
	for (int i = 0, end = allV.size(); i < end; i++) {
		vec3 t = allV[i].pos - modelCenter;
		allV[i].pos[0] = R[0][0] * t.x() + R[0][1] * t.y() + R[0][2] * t.z();
		allV[i].pos[1] = R[1][0] * t.x() + R[1][1] * t.y() + R[1][2] * t.z();
		allV[i].pos[2] = R[2][0] * t.x() + R[2][1] * t.y() + R[2][2] * t.z();
		allV[i].pos += modelCenter;
	}

#pragma omp parallel for
	for (int i = 0, end = allN.size(); i < end; i++) {
		vec3 t = allN[i];
		allN[i][0] = R[0][0] * t.x() + R[0][1] * t.y() + R[0][2] * t.z();
		allN[i][1] = R[1][0] * t.x() + R[1][1] * t.y() + R[1][2] * t.z();
		allN[i][2] = R[2][0] * t.x() + R[2][1] * t.y() + R[2][2] * t.z();
	}

#pragma omp parallel for
	for (int i = 0, end = allF.size(); i < end; i++) {
		vec3 t = allF[i].norm;
		allF[i].norm[0] = R[0][0] * t.x() + R[0][1] * t.y() + R[0][2] * t.z();
		allF[i].norm[1] = R[1][0] * t.x() + R[1][1] * t.y() + R[1][2] * t.z();
		allF[i].norm[2] = R[2][0] * t.x() + R[2][1] * t.y() + R[2][2] * t.z();
	}
}

void Model::relaseOCTNode(OCTNode* node) {
	if (node == NULL) return;
	for (int i = 0; i < 8; i++) relaseOCTNode(node->child[i]);
	delete node;
}

void Model::releaseOCTree() {
	if (octree != NULL) {
		relaseOCTNode(octree->root);
		octree = NULL;
	}
}

void Model::BuildOCTree() {
	releaseOCTree();
	octree = new OCTree();
	//更新所有的顶点位置
	int fsize = allF.size();
	for (int i = 0; i < fsize; i++) {
		allTri[i].v0 = allV[allF[i].vertexIndex[0]].pos;
		allTri[i].v1 = allV[allF[i].vertexIndex[1]].pos;
		allTri[i].v2 = allV[allF[i].vertexIndex[2]].pos;
		octree->addTri(&allTri[i]);
	}
	octree->buildTree();
}