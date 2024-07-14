#define _CRT_SECURE_NO_WARNINGS
#define FREEGLUT_STATIC

#include <gl/freeglut.h>
#include <iostream>
#include <string>
#include <math.h>

#include "Model.h"
#include "ScanLineZBuffer.h"
#include "HierarchyZbuffer.h"
#include "OCTreeHierarchyZbuffer.h"

#ifndef GL_Lib
#ifdef _DEBUG
#define GL_Lib(name) name "d.lib"
#else
#define GL_Lib(name) name ".lib"
#endif
#endif
#pragma comment(lib, GL_Lib("freeglut_static"))


std::string modelPath("./model/model.obj");
//模式选择0-ScanLineZBuffer, 1-HierarchyZBuffer, 2-OCTreeHierarchyZBuffer
int playmode = 0;

int WindowSizeWidth = 800, WindowSizeHeight = 600;
Model* showModel;
ScanLineZBuffer* zBuffer;
HierarchyZbuffer* hzBuffer;
OCTreeHierarchyZbuffer* oczBuffer;

int MouseDownPositionX = 0, MouseDownPositionY = 0;
int MouseUpPositionX = 0, MouseUpPositionY = 0;


void initAll();
void myDisplay();
void myReshape(int width, int height);
void myMouse(int button, int state, int x, int y);
void myKeyboard(unsigned char key, int x, int y);

int main(int argc, char* argv[]) {
	initAll();
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	//glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(WindowSizeWidth, WindowSizeHeight);
	glutCreateWindow("ZBuffer");
	glutDisplayFunc(myDisplay);
	glutReshapeFunc(myReshape);
	glutMouseFunc(myMouse);
	glutKeyboardFunc(myKeyboard);

	glutMainLoop();
	return 0;
}

void initAll() {
	showModel = new Model(modelPath);

	zBuffer = new ScanLineZBuffer(WindowSizeWidth, WindowSizeHeight);
	
	hzBuffer = new HierarchyZbuffer(WindowSizeWidth, WindowSizeHeight);

	oczBuffer = new OCTreeHierarchyZbuffer(WindowSizeWidth, WindowSizeHeight);
}

void myDisplay() {
	//glEnable(GL_DEPTH_TEST);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, WindowSizeWidth, 0, WindowSizeHeight);
	//glOrtho(0, WindowSizeWidth, 0, WindowSizeHeight, -500, 500);

	if (playmode == 0) {
		//扫描线z-buffer
		zBuffer->ZBufferUpdate(showModel);
		int msize = showModel->allF.size();
		glBegin(GL_POINTS);
		for (int y = 0; y < WindowSizeHeight; y++) {
			for (int x = 0; x < WindowSizeWidth; x++) {
				glColor3f(0.0, 0.0, 0.0);
				int whichPoly = zBuffer->pixelId[y][x];
				if (whichPoly >= 0 && whichPoly<msize) {
					vec3 polyColor = showModel->allF[whichPoly].color;
					glColor3f(polyColor[0], polyColor[1], polyColor[2]);
				}
				glVertex2i(x, y);
			}
		}
		glEnd();
	}
	else if (playmode == 1) {
		//层次z-Buffer
		hzBuffer->ZBufferUpdate(showModel);
		int msize = showModel->allF.size();
		glBegin(GL_POINTS);
		for (int y = 0; y < WindowSizeHeight; y++) {
			for (int x = 0; x < WindowSizeWidth; x++) {
				glColor3f(0.0, 0.0, 0.0);
				int whichPoly = hzBuffer->pixelId[y][x];
				if (whichPoly >= 0 && whichPoly < msize) {
					vec3 polyColor = showModel->allF[whichPoly].color;
					glColor3f(polyColor[0], polyColor[1], polyColor[2]);
				}
				glVertex2i(x, y);
			}
		}
		glEnd();
	}
	else if (playmode == 2) {
		//八叉树层次z-Buffer
		oczBuffer->ZBufferUpdate(showModel);
		int msize = showModel->allF.size();
		glBegin(GL_POINTS);
		for (int y = 0; y < WindowSizeHeight; y++) {
			for (int x = 0; x < WindowSizeWidth; x++) {
				glColor3f(0.0, 0.0, 0.0);
				int whichPoly = oczBuffer->pixelId[y][x];
				if (whichPoly >= 0 && whichPoly < msize) {
					vec3 polyColor = showModel->allF[whichPoly].color;
					glColor3f(polyColor[0], polyColor[1], polyColor[2]);
				}
				glVertex2i(x, y);
			}
		}
		glEnd();
	}

	//直接用glut的深度
	//glBegin(GL_TRIANGLES);
	//for (int i = 0; i < showModel->allF.size(); i++) {
	//	vec3 &color = showModel->allF[i].color;
	//	glColor3f(color[0], color[1], color[2]);
	//	for (int j = 0; j < 3; j++) {
	//		vec3& vt = showModel->allV[showModel->allF[i].vertexIndex[j]].pos;
	//		glVertex3f(vt[0], vt[1], vt[2]);
	//	}
	//}
	//glEnd();

	glutSwapBuffers();
}

void myReshape(int width, int height) {
	WindowSizeWidth = width;
	WindowSizeHeight = height;
	zBuffer->ResizeZBuffer(width, height);
	hzBuffer->ResizeZBuffer(width, height);
	oczBuffer->ResizeZBuffer(width, height);
	glViewport(0, 0, width, height);
	glutPostRedisplay();
}

void myMouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			MouseDownPositionX = x;
			MouseDownPositionY = WindowSizeHeight - 1 - y;
			//int whichpoly = hzBuffer->pixelId[MouseDownPositionY][MouseDownPositionX];
			//std::cout << whichpoly << std::endl;
			//std::cout << showModel->allF[whichpoly].vertexIndex[0] << " "<< showModel->allF[whichpoly].vertexIndex[1] <<" "<< showModel->allF[whichpoly].vertexIndex[2] << std::endl;
		}
		if (state == GLUT_UP) {
			MouseUpPositionX = x;
			MouseUpPositionY = WindowSizeHeight - 1 - y;
			double rotateX = (MouseUpPositionX - MouseDownPositionX) / 10.0;
			double rotateY = (MouseUpPositionY - MouseDownPositionY) / 10.0;
			showModel->rotateFace(-rotateX, 0.0, 1.0, 0.0);
			showModel->rotateFace(rotateY, 1.0, 0.0, 0.0);
			zBuffer->ShouldRenew = true;
			hzBuffer->ShouldRenew = true;
			oczBuffer->ShouldRenew = true;
			glutPostRedisplay();
		}
	}
}

void myKeyboard(unsigned char key, int x, int y) {
	if (key == 'c' || key == 'C') {
		if (playmode == 0) {
			playmode = 1;
			hzBuffer->ShouldRenew = true;
			std::cout << "Change Mode To HierarchyZbuffer" << std::endl;
			std::cout << std::endl;
		}
		else if (playmode == 1) {
			playmode = 2;
			oczBuffer->ShouldRenew = true;
			std::cout << "Change Mode To OCTreeZbuffer" << std::endl;
			std::cout << std::endl;
		}
		else if (playmode == 2) {
			playmode = 0;
			zBuffer->ShouldRenew = true;
			std::cout << "Change Mode To ScanLineZbuffer" << std::endl;
			std::cout << std::endl;
		}
	}
}