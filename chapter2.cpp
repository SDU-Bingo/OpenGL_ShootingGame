#include<GL/glew.h>
#include<GL/glut.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include "ObjLoader.h"
#include<math.h>
#include<time.h>
#include<iostream>

void normalKey(unsigned char, int, int);//普通按键wasd等事件
void myReshape(int, int);//窗口第一次出现时以及改变窗口大小时调用
void myDisplay();//主要显示程序
void timeFunc(int);//时间函数，每隔固定时间调用
void mouseFunc(int, int, int, int);//鼠标按键事件
void mouseMotion(int, int);//鼠标有按键按下时，移动鼠标触发
static void testDisplayList();//显示列表
static void initDisplayList();//注册显示列表
GLuint testDL;//测试显示列表，为了演示显示列表的使用方法
GLuint loadBMP_custom(const char * imagepath);//BMP图片文件读取函数，后面讲贴图会用到

struct Camera {//相机结构体，用来表示相机的位置
	glm::vec4 direction = glm::vec4(0, 0, -1, 0);
	glm::vec4 right = glm::vec4(1, 0, 0, 0);
	glm::vec4 up = glm::vec4(0, 1, 0, 0);
	glm::vec4 pos = glm::vec4(0, 0, 0, 0);
	//2-②视角变化,通过yaw，pitch算出各个朝向的值。
	void changeView(float yaw, float pitch)
	{
		glm::mat4 yawRotate = glm::rotate(glm::mat4(1.0f), glm::radians(-yaw), glm::vec3(0, 1, 0));
		//构造旋转矩阵，结果传给yawRotate，glm::rotate函数的参数含义分别为：
		//一个mat4单位矩阵，旋转角度（注意是弧度值，所以要用radians函数转换，）
		//另外注意值的正负与旋转方向的关系，旋转时所绕的轴（注意这里是vec3类型）
		//这样就得到了一个以(0,1,0)为轴旋转yaw角度的旋转矩阵
		direction = yawRotate * glm::vec4(0, 0, -1, 0);//绕(0,1,0)旋转会影响direction和right两个方向
		right = yawRotate * glm::vec4(1, 0, 0, 0);
		glm::mat4 pitchRotate = glm::rotate(glm::mat4(1.0f), glm::radians(-pitch), glm::vec3(right));
		//得到一个以right为轴（注意要以right为轴而不是(1,0,0)），pitch为旋转角度的旋转矩阵
		direction = pitchRotate * direction;//绕right旋转会影响direction和up两个方向
		up = pitchRotate * glm::vec4(0, 1, 0, 0);
	}
};
Camera camera;

struct ViewControl {
	float yaw = 0;
	float pitch = 0;
	bool rightButton = false;
	int lastX = 0, lastY = 0;
};
ViewControl viewControl;
//2-①移动
struct Move {
	bool wDown = false;
	bool aDown = false;
	bool sDown = false;
	bool dDown = false;
	void reset() {
		wDown = aDown = sDown = dDown = false;
	}
	void set(unsigned char key) {
		switch (key)
		{
		case 'w':
			wDown = true;
			break;
		case 'a':
			aDown = true;
			break;
		case 's':
			sDown = true;
			break;
		case 'd':
			dDown = true;
			break;
		default:
			break;
		}
	}
	glm::vec4 moveVector(Camera &camera)//返回本次移动的向量
	{
		if (!(wDown | aDown | sDown | dDown))//没有按键按下
			return glm::vec4(0,0,0,0);
		else
		{
			if (wDown)
				return camera.direction;//w按下时，向相机的朝向移动
			if (aDown)
				return -camera.right;//a按下，向相机右向的反方向移动
			if (sDown)
				return -camera.direction;//s按下，向相机朝向的反方向移动
			if (dDown)
				return camera.right;//d按下，向右方向移动
		}
	}
};
Move move;
int windowsWidth,windowsHeight;//窗口的大小参数
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);//彩色显示，双缓冲
	glutInitWindowPosition(200, 50);
	glutInitWindowSize(1000, 1000);
	glutCreateWindow("Tutorial");
	initDisplayList();//初始化显示列表要在创造窗口之后调用
	//1-①功能函数的绑定
	glutDisplayFunc(&myDisplay);
	glutReshapeFunc(&myReshape);
	glutKeyboardFunc(&normalKey);
	glutMotionFunc(&mouseMotion);
	glutMouseFunc(&mouseFunc);
	glutTimerFunc(16, &timeFunc, 1);//每16毫秒调用一次该函数，保证输出60帧的动画
	glutMainLoop();//开始显示
	return 0;
}
void normalKey(unsigned char key, int x, int y)
{
	move.set(key);
}
void myReshape(int w, int h)
{
	windowsWidth = w;
	windowsHeight = h;
	//1-②透视投影实现，一定要按照按以下顺序，否则会显示不正确。
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(95.0, (GLfloat)w / (GLfloat)h, 1, 100000.0);//四个参数含义分别为视野角度，视野宽度，视野高度，能看到的最近距离，能看到的最远距离
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
		camera.pos.x, camera.pos.y, camera.pos.z,//相机位置
		camera.pos.x + camera.direction.x, camera.pos.y + camera.direction.y, camera.pos.z + camera.direction.z,//相机的朝向向量
		camera.up.x, camera.up.y, camera.up.z//相机的顶部朝向向量
	);
}
//1-③显示列表的使用，在大量绘画重复元素时使用显示列表能够有效加快绘制速度
static void testDisplayList()
{
	glPushMatrix();//压栈使本次绘制的矩阵变换不影响后面的操作
	glTranslatef(-100, 0, -100);//移动到该点
	glColor3f(1, 0, 0);//使用红色
	glutSolidCube(10);//绘制正方体
	glPopMatrix();//临时矩阵出栈

	glPushMatrix();
	glTranslatef(100, 0, -100);
	glColor3f(0, 1, 0);
	glutSolidCube(10);
	glPopMatrix(); 

	glPushMatrix();
	glTranslatef(100, 0, 100);
	glColor3f(0, 0, 1);
	glutSolidCube(10);
	glPopMatrix(); 

	glPushMatrix();
	glTranslatef(-100, 0, 100);
	glColor3f(1, 1, 1);
	glutSolidCube(10);
	glPopMatrix();
}
static void initDisplayList()
{
	testDL = glGenLists(1);//为testDL显示列表分配空间
	glNewList(testDL, GL_COMPILE);//为testDL绑定函数
	testDisplayList();//绘制工作在本函数中完成，注意不能向其传递变量
	glEndList();//结束绑定
}
void timeFunc(int value)
{
	glutPostRedisplay();//该命令调用myDisplay函数
	glutTimerFunc(16, timeFunc, 1);//配合上一条命令实现每秒1000/16=60次绘制
}
void mouseFunc(int button, int state, int x, int y)//2-②视角移动，处理右键按下的情况
{
	switch (button)
	{
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN)//当鼠标右键按下时
		{
			viewControl.rightButton = true;
			viewControl.lastX = x;//记录右键按下时鼠标的位置
			viewControl.lastY = y;
		}
		else 
			viewControl.rightButton = false;//右键抬起时，置按下状态为假
		break;
	default:
		break;
	}
}
void mouseMotion(int x, int y)//2-②视角移动，处理鼠标移动
{
	if (viewControl.rightButton)//通过每次移动的距离得到当前的yaw值和pitch值
	{
		viewControl.yaw += (x - viewControl.lastX)*0.7;//*0.7为调整灵敏度
		viewControl.pitch += (y - viewControl.lastY)*0.7;
		if (viewControl.yaw > 180)//横向角度范围0-360，超界后如下处理可以保证视角连贯变化
			viewControl.yaw = -180;
		else if (viewControl.yaw < -180)
			viewControl.yaw = 180;
		if (viewControl.pitch > 75)//纵向范围-75到75，超界后限制在最大值
			viewControl.pitch = 75;
		else if (viewControl.pitch < -75)
			viewControl.pitch = -75;
		viewControl.lastX = x;
		viewControl.lastY = y;
		//得到角度后，计算camera的direction、right、up的值就可以得到新的朝向
		camera.changeView(viewControl.yaw, viewControl.pitch);
	}
}
void myDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT);//每次绘制前要清空颜色缓冲区
	glClear(GL_DEPTH_BUFFER_BIT);//每次绘制前要清空深度缓冲区
	glLoadIdentity();//设置视角前要置位矩阵
	camera.pos = camera.pos + move.moveVector(camera)*3.0f;//二-①计算本次相机位置,*3.0f为速度，注意vec4类型只能与float类型直接相乘
	move.reset();//二-①复位按键状态
	gluLookAt(
		camera.pos.x, camera.pos.y, camera.pos.z,
		camera.pos.x + camera.direction.x, camera.pos.y + camera.direction.y, camera.pos.z + camera.direction.z,
		camera.up.x, camera.up.y, camera.up.z
	);
	glCallList(testDL);//调用显示列表，绘制场景
	glutSwapBuffers();//因为是双缓冲，所以要用swapbuffer显示动画
}
GLuint loadBMP_custom(const char * imagepath)
{
	unsigned char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int width, height;
	unsigned int imageSize;   // = width*height*3
							  // Actual RGB data
	unsigned char * data;
	FILE * file = fopen(imagepath, "rb");
	if (!file) { printf("Image could not be opened\n"); return 0; }
	if (fread(header, 1, 54, file) != 54) { // If not 54 bytes read : problem
		printf("Not a correct BMP file\n");
		return false;
	}
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		return 0;
	}
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);
	if (imageSize == 0)    imageSize = width * height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way
	data = new unsigned char[imageSize];
	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);
	//Everything is in memory now, the file can be closed
	fclose(file);
	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	return textureID;
}