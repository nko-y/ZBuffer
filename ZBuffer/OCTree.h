#pragma once

#include "vec3.h"
#include <iostream>

class Bound
{
public:
	Bound() {
		min_point = { INT_MAX, INT_MAX, INT_MAX };
		max_point = { INT_MIN, INT_MIN, INT_MIN };
	}

	Bound(double minx, double miny, double minz, double maxx, double maxy, double maxz) {
		min_point = vec3(minx, miny, minz);
		max_point = vec3(maxx, maxy, maxz);
	}

	Bound(Triangle& tri) {
		vec3& v0 = tri.v0;
		vec3& v1 = tri.v1;
		vec3& v2 = tri.v2;
		min_point[0] = std::min(v0[0], std::min(v1[0], v2[0]));
		min_point[1] = std::min(v0[1], std::min(v1[1], v2[1]));
		min_point[2] = std::min(v0[2], std::min(v1[2], v2[2]));
		max_point[0] = std::max(v0[0], std::max(v1[0], v2[0]));
		max_point[1] = std::max(v0[1], std::max(v1[1], v2[1]));
		max_point[2] = std::max(v0[2], std::max(v1[2], v2[2]));
	}

	~Bound() {};

	void union_bound(Bound& b) {
		min_point[0] = std::min(min_point[0], b.min_point[0]);
		min_point[1] = std::min(min_point[1], b.min_point[1]);
		min_point[2] = std::min(min_point[2], b.min_point[2]);
		max_point[0] = std::max(max_point[0], b.max_point[0]);
		max_point[1] = std::max(max_point[1], b.max_point[1]);
		max_point[2] = std::max(max_point[2], b.max_point[2]);
	}

	void union_triangle(Triangle& tri) {
		vec3& v0 = tri.v0;
		vec3& v1 = tri.v1;
		vec3& v2 = tri.v2;
		min_point[0] = std::min(std::min(v0[0], min_point[0]), std::min(v1[0], v2[0]));
		min_point[1] = std::min(std::min(v0[1], min_point[1]), std::min(v1[1], v2[1]));
		min_point[2] = std::min(std::min(v0[2], min_point[2]), std::min(v1[2], v2[2]));
		max_point[0] = std::max(std::max(v0[0], max_point[0]), std::max(v1[0], v2[0]));
		max_point[1] = std::max(std::max(v0[1], max_point[1]), std::max(v1[1], v2[1]));
		max_point[2] = std::max(std::max(v0[2], max_point[2]), std::max(v1[2], v2[2]));
	}

	bool isIntersectBound(Bound& b) {
		if (std::max(min_point[0], b.min_point[0]) > std::min(max_point[0], b.max_point[0]) ||
			std::max(min_point[1], b.min_point[1]) > std::min(max_point[1], b.max_point[1]) ||
			std::max(min_point[2], b.min_point[2]) > std::min(max_point[2], b.max_point[2]))
			return false;
		return true;
	}

	bool isIntersectTri(Triangle& tri) {
		Bound tp(tri);
		return isIntersectBound(tp);
	}

	void print_bound() {
		std::cout << min_point[0] << " " << min_point[1] << " " << min_point[2] << std::endl;
		std::cout << max_point[0] << " " << max_point[1] << " " << max_point[2] << std::endl;
	}

	vec3 min_point, max_point;
};

class OCTNode
{
public:
	OCTNode() = default;
	OCTNode(Bound& bound) {
		b = bound;
		for (int i = 0; i < 8; i++) child[i] = NULL;
	}

	~OCTNode() {};

	void addTri(Triangle* tri) {
		allTri.push_back(tri);
	}

	bool isLeaf() {
		for (int i = 0; i < 8; i++) {
			if (child[i] != NULL) return false;
		}
		return true;
	}

	void splitNode(int maxTriNum);

	//节点边界信息
	Bound b;
	OCTNode* child[8];
	std::vector<Triangle*> allTri;
};

class OCTree
{
public:
	OCTree() {
		root = NULL;
	}
	~OCTree() {};

	void buildTree();
	void addTri(Triangle* one) {
		allTri.push_back(one);
	}

	int maxTriNum = 50;
	std::vector<Triangle*> allTri;
	OCTNode* root;
};

