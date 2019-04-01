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
void setLight();//三-①光源的设置
void moveLight();//三-①光源的调整，OpenGL中光源的位置需要单独控制，否则会出现意外结果
void genGrass();//三-②生成草地
void readTexture();//读取纹理图片
void drawSkyBox(GLuint);
static void testDisplayList();//显示列表
static void initDisplayList();//注册显示列表
GLuint testDL;//测试显示列表，为了演示显示列表的使用方法
GLuint loadBMP_custom(const char * imagepath);//BMP图片文件读取函数，后面讲贴图会用到
GLuint skyBoxFront, skyBoxLeft, skyBoxRight, skyBoxBack, skyBoxTop, grassTexture;//三-②绑定纹理用的变量
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
struct Grass//三-②生成草地
{
	GLfloat grass_vert[101][101][3];//草地顶点数组,共101*101个点，每个点3个值
	GLuint grass_list[100][100][6];//顶点数组连接顺序，共100*100个格子即20000个三角形，每个三角形3个点
	GLfloat grass_norm[101][101][3];//顶点法线数组同顶点
	GLfloat grass_coor[101][101][2];//顶点纹理坐标101*101个点，每个点两个纹理坐标
	const int hillNum = 50;//山丘数量
	const int meshNum = 100;//格子数量
	const int meshNum1 = 101;//顶点数量
	int hill_coord[50][3];//山丘中心店坐标
	float getHeight(float x, float y)//当得到山丘的种子后，通过该函数获得地图上任意一点的地形高度
	{
		float hillHeight = 0;
		for (int v0 = 0; v0 < 101; v0++)
		{
			float temp = hill_coord[v0][2] - (x + 5000 - hill_coord[v0][0])*(x + 5000 - hill_coord[v0][0]) / 5000 - (y + 5000 - hill_coord[v0][1])*(y + 5000 - hill_coord[v0][1]) / 5000;
			hillHeight += (temp > 0 ? temp : 0);
		}
		return hillHeight - 500;
	}
	void genGrass()//三-②生成草地
	{
		srand((unsigned)time(NULL));//随机数种子
		for (int v = 0; v < hillNum; v++)
		{
			//下面的指令即为山丘中心点的生成代码，注意我们网格的范围是0-10000（即101个点有100个间隔，每个间隔为100）
			hill_coord[v][0] = rand() % 8000 + 1000;//在1000-9000的范围内随机生成中心点的横坐标
			hill_coord[v][1] = rand() % 8000 + 1000;//在1000-9000的范围内随机生成中心点的纵坐标
			hill_coord[v][2] = rand() % 200 + 100;//在100到300的范围内随机生成中心点高度
		}
		for (int v = 0; v < meshNum1; v++)
			for (int v1 = 0; v1 < meshNum1; v1++)
			{
				float hillHeight = 0;
				for (int v2 = 0; v2 < hillNum; v2++)
				{
					//计算每一个顶点的高度，即对每一个山丘种子计算抛物面在该点的值，小于0则统一为零，最后叠加起来
					//z=c-(x^2/a+y^2/b)
					//每一个种子在该点的贡献为，该点的高度 减去 任一顶点到该点的水平距离的平方除以一个常数（可以自行调整，该常数决定了坡度大小）
					float temp = hill_coord[v2][2] - (v * 100 - hill_coord[v2][0])*(v * 100 - hill_coord[v2][0]) / 5000 - (v1 * 100 - hill_coord[v2][1])*(v1 * 100 - hill_coord[v2][1]) / 5000;
					hillHeight += (temp > 0 ? temp : 0);
				}
				grass_vert[v][v1][0] = v * 100 - 5000;
				grass_vert[v][v1][1] = hillHeight - 500;
				grass_vert[v][v1][2] = v1 * 100 - 5000;
				//将网格中心移动到0,0,0
				grass_coor[v][v1][0] = abs((v % 18) - 9) / 9.0;
				grass_coor[v][v1][1] = abs((v1 % 18) - 9) / 9.0;
				//每九个格对应一张图片且每个图片的边缘是连续的，比较难理解但是很重要，可以自行尝试其他纹理坐标查看效果
				if (v1 < meshNum && v < meshNum)
				{
					//对于第[v][v1]个点，需要和[v][v1+1],[v+1][v1]相连，此外还需要[v][v1+1],[v+1][v1]与[v+1][v1+1]相连，因为有20000个三角形
					grass_list[v][v1][0] = v * meshNum1 + v1;//[v][v1]
					grass_list[v][v1][1] = v * meshNum1 + v1 + 1;//[v][v1+1]
					grass_list[v][v1][2] = (v + 1) * meshNum1 + v1;//[v+1][v1]

					grass_list[v][v1][3] = (v + 1) * meshNum1 + v1;//[v+1][v1]
					grass_list[v][v1][4] = v * meshNum1 + v1 + 1;//[v][v+1]
					grass_list[v][v1][5] = (v + 1) * meshNum1 + v1 + 1;//[v+1][v1+1]
				}
			}
		for (int v = 1; v < meshNum; v++)
			for (int v1 = 1; v1 < meshNum; v1++)
			{
				//每一个点的法向量由它上下两个点组成的向量和它左右两个点组成的向量叉乘得到
				glm::vec3 vec1(grass_vert[v - 1][v1][0], grass_vert[v - 1][v1][1], grass_vert[v - 1][v1][2]);
				glm::vec3 vec2(grass_vert[v + 1][v1][0], grass_vert[v + 1][v1][1], grass_vert[v + 1][v1][2]);
				glm::vec3 vec3(grass_vert[v][v1 - 1][0], grass_vert[v][v1 - 1][1], grass_vert[v][v1 - 1][2]);
				glm::vec3 vec4(grass_vert[v][v1 + 1][0], grass_vert[v][v1 + 1][1], grass_vert[v][v1 + 1][2]);
				glm::vec3 temp = glm::normalize(glm::cross(vec4 - vec3, vec2 - vec1));//对叉乘结果归一化
				grass_norm[v][v1][0] = temp.x;
				grass_norm[v][v1][1] = temp.y;
				grass_norm[v][v1][2] = temp.z;
			}
		for (int v = 0; v<meshNum1; v++)
			for (int v1 = 0; v1 < meshNum1; v1++)
			{
				if (v == 0 || v == meshNum1 || v1 == 0 || v1 == meshNum1)
				{
					//对边缘的法向量全部赋值为0，因此实际效果中边缘由于不发光会发黑。
					grass_norm[v][v1][0] = 0;
					grass_norm[v][v1][1] = 0;
					grass_norm[v][v1][2] = 0;
				}
			}
	}
};
Grass grass;
int windowsWidth,windowsHeight;//窗口的大小参数
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);//彩色显示，双缓冲
	glutInitWindowPosition(200, 50);
	glutInitWindowSize(1000, 1000);
	glutCreateWindow("Tutorial");
	initDisplayList();//初始化显示列表要在创造窗口之后调用
	setLight();//三-①光源的设置
	glEnable(GL_DEPTH_TEST);//开启深度测试
	readTexture();//三-②读取纹理
	grass.genGrass();
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
	//glutSolidCube(10);//绘制正方体
	glutSolidSphere(10, 64, 64);
	glPopMatrix();//临时矩阵出栈

	glPushMatrix();
	glTranslatef(100, 0, -100);
	glColor3f(0, 1, 0);
	//glutSolidCube(10);
	glutSolidSphere(10, 64, 64);
	glPopMatrix(); 

	glPushMatrix();
	glTranslatef(100, 0, 100);
	glColor3f(0, 0, 1);
	//glutSolidCube(10);
	glutSolidSphere(10, 64, 64);
	glPopMatrix(); 

	glPushMatrix();
	glTranslatef(-100, 0, 100);
	glColor3f(1, 1, 1);
	//glutSolidCube(10);
	glutSolidSphere(10, 64, 64);
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
		viewControl.yaw += (x - viewControl.lastX)*0.3;//*0.7为调整灵敏度
		viewControl.pitch += (y - viewControl.lastY)*0.3;
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
void setLight()//三-①光源的设置
{
	GLfloat mat_specular[] = { 0.0, 0.0, 0.0, 1.0 };  //镜面反射参数
	GLfloat mat_diffuse[] = { 1, 1, 1, 1.0 };  //漫反射参数
	GLfloat mat_shininess[] = { 0.0 };         //高光指数
	GLfloat light1_angle[] = { 15.0 };//LIGHT1扩散角度
	GLfloat light_position[] = { 0.0, 1.0, -1.0, 0.0 };//LIGHT0位置,最后一个值为0则为平行光
	GLfloat white_light[] = { 0.3, 0.3, 0.3, 1.0 };//LIGHT0参数，灯颜色(0.3,0.3,0.3), 第四位为开关
	GLfloat yellow_light[] = { 1,0.87,0.315,1.0 };//LIGHT1参数
	GLfloat Light_Model_Ambient[] = { 0.1, 0.1, 0.1, 1.0 }; //环境光参数
	glClearColor(0.0, 0.0, 0.0, 0.0);  //背景色
	glShadeModel(GL_SMOOTH);           //多变性填充模式
    //材质属性
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	//平行光LIGHT0设置
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);   //散射光属性
	glLightfv(GL_LIGHT0, GL_SPECULAR, mat_specular);  //镜面反射光
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Light_Model_Ambient);  //环境光参数 
	//锥状光源LIGHT1设置
	GLfloat lightKc = 1, lightKl = 0.0, lightKq = 0.0, lightExp = 2;//有关点光源扩散强度衰减的参数。
	glLightfv(GL_LIGHT1, GL_CONSTANT_ATTENUATION, &lightKc);
	glLightfv(GL_LIGHT1, GL_LINEAR_ATTENUATION, &lightKl);
	glLightfv(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, &lightKq);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellow_light);
	glLightfv(GL_LIGHT1, GL_SPECULAR, mat_specular);  
	glLightfv(GL_LIGHT1, GL_SPOT_CUTOFF, light1_angle);
	glLightfv(GL_LIGHT1, GL_SPOT_EXPONENT, &lightExp);
}
//三-①光源设置
void moveLight()
{
	glPushMatrix();
	//设置LIGHT0
	GLfloat light0_position[] = { 0.0, 1.0, -1.0, 0.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	//设置LIGHT1
	GLfloat light1_position[] = { 0,0,0,1 };
	GLfloat light1_direction[] = { 0,0,0 };
	light1_position[0] = camera.pos.x;//根据相机位置设置LIGHT1位置
	light1_position[1] = camera.pos.y;
	light1_position[2] = camera.pos.z;
	light1_direction[0] = camera.direction.x;//根据相机朝向设置LIGHT1朝向
	light1_direction[1] = camera.direction.y;
	light1_direction[2] = camera.direction.z;
	glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
	glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, light1_direction);
	glPopMatrix();

}
void drawGrass(GLuint face)//三-②草地的绘制
{
	glBindTexture(GL_TEXTURE_2D, face);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, grass.grass_vert);
	glTexCoordPointer(2, GL_FLOAT, 0, grass.grass_coor);
	glNormalPointer(GL_FLOAT, 0, grass.grass_norm);
	glDrawElements(GL_TRIANGLES, 60000, GL_UNSIGNED_INT, grass.grass_list);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisable(GL_TEXTURE_2D);
}
void drawScene()//三-②使用顶点数组绘制天空盒
{
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);//设置纹理属性，当最后一项为GL_REPLACE时，贴图效果不受环境光影响
	//下面的代码通过传递不同面的贴图id实现绘画不同面的天空，注意不同系统下读取图片的方向
	//不一定相同，所以必要时需要查看实际结果对图形进行旋转才能得到合理的结果
	glPushMatrix();
	glTranslatef(camera.pos.x, camera.pos.y, camera.pos.z);//使天空盒和玩家距离适中相等，可以提高真实感
	drawSkyBox(skyBoxFront);
	glRotatef(90, 0, 1, 0);
	drawSkyBox(skyBoxLeft);
	glRotatef(90, 0, 1, 0);
	drawSkyBox(skyBoxBack);
	glRotatef(90, 0, 1, 0);
	drawSkyBox(skyBoxRight);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(camera.pos.x, camera.pos.y, camera.pos.z);//使天空盒和玩家距离适中相等，可以提高真实感
	//glRotatef(90, 0, 1, 0);
	glRotatef(90, 1, 0, 0);
	drawSkyBox(skyBoxTop);
	glPopMatrix();
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	drawGrass(grassTexture);
}
void drawSkyBox(GLuint textureID)//三-②使用顶点数组绘制天空盒
{
	GLfloat wall_vert[][3] = {//坐标数组，每三个数据为一组
		-50000,-50000,-50000,
		50000,-50000,-50000,
		50000,50000,-50000,
		-50000,50000,-50000,
	};
	GLubyte index_list[][4] = {//顶点列表，按该表顺序连接上面的各个坐标点
		0,1,2,3,
	};
	GLfloat tex_coor[][2] = {//对应每个坐标点的纹理坐标
		0,0,//第一个点对应图像左上角
		1,0,//第二个点对应图像左下角
		1,1,//第三个点对应图像右下角
		0,1,//第四个点对应图像右上角
		//以上对应关系可能有误，需要理解不同系统下图像坐标的表示方法
	};
	glBindTexture(GL_TEXTURE_2D, textureID);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, wall_vert);
	glTexCoordPointer(2, GL_FLOAT, 0, tex_coor);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_BYTE, index_list);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}
void readTexture()//三-②读取纹理
{
	//注意文件路径，以可执行文件根目录开始或者写上绝对路径
	skyBoxFront = loadBMP_custom("source\\front.bmp");
	skyBoxBack = loadBMP_custom("source\\back.bmp");
	skyBoxLeft = loadBMP_custom("source\\left.bmp");
	skyBoxRight = loadBMP_custom("source\\right.bmp");
	skyBoxTop = loadBMP_custom("source\\top.bmp");
	grassTexture = loadBMP_custom("source\\grass.bmp");
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
	moveLight();//调整光源的位置，包括LIGHT0与LIGHT1
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glCallList(testDL);//调用显示列表，绘制场景
	drawScene();
	glutSwapBuffers();//因为是双缓冲，所以要用swapbuffer显示动画
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHTING);
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