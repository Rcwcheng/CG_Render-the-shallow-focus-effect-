// ConsoleApplication1.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//
#include <glad/glad.h>
#include <GL/freeglut.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <string>
#include <fstream>
#include <vector>
#define GLM_FORCE_RADIAN
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <functional>


using namespace std;

#define WINDOW_TITLE_PREFIX "Advanced CG Example"

int CurrentWidth = 800, CurrentHeight = 600, WindowHandle = 0;
const aiScene *scene;

struct ObjectData {
	GLuint vao;
	unsigned int ibo_count;
	float* transform;
};

ObjectData myobj;
struct Framebuffer {
	GLuint id;
	GLuint color_id, depth_id;
};
Framebuffer fbo_scene;


void ResizeFunction(int, int);
void RenderFunction();
void KeyboardFunction(unsigned char key, int x, int y);
void MouseFunction(int, int, int, int);
void MotionFunction(int, int);
void RenderScene(const aiScene* sc, const aiNode* nd);
GLuint CreateShaderProgram(string vsfile, string fsfile);
ObjectData CreateStaticMesh(const aiMesh* mesh, float *);
Framebuffer CreateFramebuffer();

// camera parameters
float CameraFOV = 45;
float CameraY = 1000;
int mouseX, mouseY;
bool dolly = false;
float dollyFactor = 0;
int lighty = 100;
float focal = 100.f;

constexpr double pi = 3.1415926;

// shader related
GLuint program, program2;


int main(int argc, char *argv[])
{
	
	printf("Loading scene...\n");
	Assimp::Importer importer;
	scene = importer.ReadFile("Knight.ply", aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

	//初始化 glut
	glutInit(&argc, argv);
	glutInitContextVersion(3, 3);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_CORE_PROFILE);

	//設定 glut 畫布尺寸 與color / depth模式
	glutInitWindowSize(CurrentWidth, CurrentHeight);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);

	//根據已設定好的 glut (如尺寸,color,depth) 向window要求建立一個視窗，接著若失敗則退出程式
	WindowHandle = glutCreateWindow(WINDOW_TITLE_PREFIX);
	if (WindowHandle < 1) { fprintf(stderr, "ERROR: Could not create a new rendering window.\n"); exit(EXIT_FAILURE); }
	
	if (!gladLoadGL()) {
		exit(EXIT_FAILURE);
	}



	fbo_scene = CreateFramebuffer();
	
	//建mesh
	myobj = CreateStaticMesh(scene->mMeshes[scene->mRootNode->mMeshes[0]],  (float *)&scene->mRootNode->mTransformation);

	glutReshapeFunc(ResizeFunction); //設定視窗 大小若改變，則跳到"AResizeFunction"這個函數處理應變事項
	glutDisplayFunc(RenderFunction);  //設定視窗 如果要畫圖 則執行"RenderFunction"
	glutKeyboardFunc(KeyboardFunction);
	//glutMouseFunc(MouseFunction);
	//glutMotionFunc(MotionFunction);
	//glutPassiveMotionFunc(MotionFunction);

	// Before we move on, enable necessary setting
	// since there's default color on each node in the scene
	// uncomment the following code if you want lighting
	/*
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	GLfloat MaterialAmbient[] = { 0.4,0.4,0.4,1.0f };
	GLfloat MaterialDiffuse[] = { 0.7,0.7,0.7,1.0f };
	GLfloat MaterialSpecular[] = { 1.2,1.2,1.2, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, MaterialAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, MaterialDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, MaterialSpecular);
	
	glEnable(GL_COLOR_MATERIAL);
	*/
	
	//讀2個shader: (1)畫3D/打光 (2)2D後處理
	program = CreateShaderProgram("vs.vert", "fs.frag");
	program2 = CreateShaderProgram("vs_p.vert", "fs_p.frag");
	glutMainLoop();
	

	exit(EXIT_SUCCESS);
}

void ResizeFunction(int Width, int Height)
{
	CurrentWidth = Width;
	CurrentHeight = Height;
	glViewport(0, 0, CurrentWidth, CurrentHeight);
}

//Dolly Zoom & 鍵盤指令
void KeyboardFunction(unsigned char key, int x, int y)
{
	switch (tolower(key)) {
	case 'w':
		CameraY -= 10; break;
	case 's':
		CameraY += 10; break;
	case 'q':
		// toggle dolly zoom(on/off)
		dolly = !dolly;
		// if, currently, dolly is toggled to on
		if (dolly) {
			dollyFactor = 1 / tan(CameraFOV * pi / 180 / 2) / CameraY;
			printf("Dolly enabled\n");
		}
		else {
			printf("Dolly disabled\n");
		}
	case 'e':
		lighty += 25; break;
	case 'r':
		lighty -= 25; break;
	case 'p':
		focal += 25; break;
	case 'o':
		focal -= 25; break;
	}
	fprintf(stderr, "%f", CameraY);


}
static std::string read_file(const std::string &filename)
{
	std::ifstream t(filename);
	if (!t.good())
		throw std::runtime_error(filename + " not found!");
	return std::string((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());
}

GLuint CreateShaderProgram(string vsfile, string fsfile) {
	GLuint vid = glCreateShader(GL_VERTEX_SHADER);
	GLuint fid = glCreateShader(GL_FRAGMENT_SHADER);

	std::string vs = read_file(vsfile);
	const char *c = vs.c_str();
	int len = vs.length();
	glShaderSource(vid, 1, &c, &len);
	glCompileShader(vid);

	int status;
	glGetShaderiv(vid, GL_COMPILE_STATUS, &status);
	
	if (status == GL_FALSE) {
		int info_len;
		glGetShaderiv(vid, GL_INFO_LOG_LENGTH, &info_len);

		std::vector<char> err_str(info_len);
		glGetShaderInfoLog(vid, info_len, &info_len, err_str.data());
		fprintf(stderr, "vs error %s\n", err_str.data());
		exit(EXIT_FAILURE);
	}

	std::string fs = read_file(fsfile);
	
	c = fs.c_str();
	len = fs.length();

	glShaderSource(fid, 1, &c, &len);
	glCompileShader(fid);
	glGetShaderiv(fid, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
		int info_len;
		glGetShaderiv(fid, GL_INFO_LOG_LENGTH, &info_len);

		std::vector<char> err_str(info_len);
		glGetShaderInfoLog(fid, info_len, &info_len, err_str.data());
		fprintf(stderr, "fs error %s\n", err_str.data());
		exit(EXIT_FAILURE);
	}

	GLuint prog = glCreateProgram();

	glAttachShader(prog, vid);
	glAttachShader(prog, fid);

	glLinkProgram(prog);

	glDeleteShader(vid);
	glDeleteShader(fid);

	return prog;

}
/*
void MouseFunction(int button, int state, int x, int y) {
	mouseX = x;
	mouseY = y;
}

void MotionFunction(int x, int y) {
	mouseX = x;
	mouseY = y;
}*/

//vbo存bufferdata
ObjectData CreateStaticMesh(const aiMesh* mesh, float *transform) {
	GLuint vao;
	glGenVertexArrays(1, &vao);
	

	// vao -> n vbo?
	glBindVertexArray(vao);

	GLuint vbo1, vbo2, vbo3;
	glGenBuffers(1, &vbo1);
	glGenBuffers(1, &vbo2);
	glGenBuffers(1, &vbo3);

	glBindBuffer(GL_ARRAY_BUFFER, vbo1);

	//STATIC_DRAW: 告訴電腦BUFFER怎麼用
	glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(aiVector3D), mesh->mVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo2);
	glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(aiVector3D), mesh->mNormals, GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo3);
	glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(aiColor4D), mesh->mColors[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, 0);


	//整理index資料的格式
	vector<int> ibo_buffer(mesh->mNumFaces * 3);
	int c = 0;
	for (int t = 0; t < mesh->mNumFaces; ++t) {
		const aiFace* face = &mesh->mFaces[t];
		GLenum face_mode;

		if (face->mNumIndices != 3)
			exit(EXIT_FAILURE);

		for (int i = 0; i < face->mNumIndices; i++) {
			int index = face->mIndices[i];
			ibo_buffer[c++] = index;
		}
	}

	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibo_buffer.size() * sizeof(int), ibo_buffer.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);

	return { vao , (unsigned int)ibo_buffer.size(), transform };
}

Framebuffer CreateFramebuffer() {
	GLuint fbo;
	glGenFramebuffers(1, &fbo);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	GLuint color_id;
	glGenTextures(1, &color_id);
	glBindTexture(GL_TEXTURE_2D, color_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CurrentWidth, CurrentHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	//int color_nvg = nvglCreateImageFromHandleGL3(vg, color_id, CurrentWidth, CurrentHeight, NVG_IMAGE_NODELETE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_id, 0);

	GLuint depth_id;
	glGenTextures(1, &depth_id);
	glBindTexture(GL_TEXTURE_2D, depth_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, CurrentWidth, CurrentHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_id, 0);


	GLuint draw_buffers[] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, draw_buffers);
	
	//檢查FRAMEBUFFER
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE)
		exit(EXIT_FAILURE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return { fbo, color_id, depth_id };
}


void RenderFunction()
{


	glBindFramebuffer(GL_FRAMEBUFFER, fbo_scene.id);
	glViewport(0, 0, CurrentWidth, CurrentHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	

	glEnable(GL_DEPTH_TEST);

	if (dolly) {
		// update camera fov
		CameraFOV = atan(1 / (dollyFactor * CameraY)) * 2 * 180 / pi;
		fprintf(stderr, "%f", CameraFOV);
	}

	glUseProgram(program);
	GLint loc_vp = glGetUniformLocation(program, "vp");
	glm::mat4 proj = glm::perspective(CameraFOV * 3.1415f / 180, (float)CurrentWidth / CurrentHeight, 1.f, 1000.0f);
	glm::mat4 view = glm::lookAt(glm::vec3{ 0.0f, CameraY, 100.f }, glm::vec3{ 0.f, CameraY - 1, 100.f }, glm::vec3{ 0.f, 0.f, 1.f });

	glm::mat4 vp = proj * view;
	glUniformMatrix4fv(loc_vp, 1, GL_FALSE, glm::value_ptr(vp));
	// m00 m01 m02 m03
	// m10 m11....

	// m00 m01 m02 m03 m10 11 row major
	// m00 m10 m20 m30 m01 m11.. col major
	GLint loc = glGetUniformLocation(program, "model");
	glUniformMatrix4fv(loc, 1, GL_TRUE, myobj.transform);
	glBindVertexArray(myobj.vao);
	glDrawElements(GL_TRIANGLES, myobj.ibo_count, GL_UNSIGNED_INT, (void *)0);

	glm::mat4 transform_ = glm::translate(glm::mat4(1.0f), glm::vec3(100, 100, 0));
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(transform_));
	glDrawElements(GL_TRIANGLES, myobj.ibo_count, GL_UNSIGNED_INT, (void*)0);


	transform_ = glm::translate(glm::mat4(1.0f), glm::vec3(-100, -100, 0));
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(transform_));
	glDrawElements(GL_TRIANGLES, myobj.ibo_count, GL_UNSIGNED_INT, (void*)0);

	//lighty是shader裡面的一個變數
	GLint loc_lighty = glGetUniformLocation(program, "lighty");
	glUniform1i(loc_lighty, lighty);
	
	/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (dolly) {
		// update camera fov
		CameraFOV = atan(1 / (dollyFactor * CameraY)) * 2 * 180 / pi;
	}

	gluPerspective(CameraFOV, (double)CurrentWidth / CurrentHeight, 0.1, 100000.);
	

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, CameraY, 100, 0, CameraY-1, 100, 0, 0, 1);

	RenderScene(scene, scene->mRootNode);
	*/

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, CurrentWidth, CurrentHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glBindVertexArray(myobj.vao);
	glUseProgram(program2);
	GLint loc_viewport = glGetUniformLocation(program2, "viewport");
	glUniform2f(loc_viewport, CurrentWidth, CurrentHeight);

	GLint loc_text = glGetUniformLocation(program2, "text");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbo_scene.color_id);
	glUniform1i(loc_text, 0);

	GLint loc_dtext = glGetUniformLocation(program2, "dtext");
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fbo_scene.depth_id);
	glUniform1i(loc_dtext, 1);

	// 1+(depth-focal)^2 / c
	// sigma 1->20

	GLint loc_t = glGetUniformLocation(program2, "focal");
	glUniform1f(loc_t, focal);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glutSwapBuffers();
	glutPostRedisplay();
}


/*
void RenderScene(const aiScene* sc, const aiNode* nd)
{
	unsigned int i;
	unsigned int n = 0, t;
	aiMatrix4x4 m = nd->mTransformation;

	// update transform
	aiMatrix4x4 mT = m.Transpose();
	glPushMatrix();
	glMultMatrixf((float*)& mT);

	// draw all meshes assigned to this node
	for (n = 0; n < nd->mNumMeshes; n++) {
		const aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];


		for (t = 0; t < mesh->mNumFaces; ++t) {
			const aiFace* face = &mesh->mFaces[t];
			GLenum face_mode;

			switch (face->mNumIndices) {
			case 1: face_mode = GL_POINTS; break;
			case 2: face_mode = GL_LINES; break;
			case 3: face_mode = GL_TRIANGLES; break;
			default: face_mode = GL_POLYGON; break;
			}

			glBegin(face_mode);
			for (i = 0; i < face->mNumIndices; i++) {
				int index = face->mIndices[i];
				if (mesh->mColors[0] != NULL) glColor4fv((GLfloat*)& mesh->mColors[0][index]);
				if (mesh->mNormals != NULL)
				{
					glNormal3fv(&mesh->mNormals[index].x);
				}

				glVertex3fv(&mesh->mVertices[index].x);
			}

			glEnd();
		}

	}

	// draw all children
	for (n = 0; n < nd->mNumChildren; ++n) {
		RenderScene(sc, nd->mChildren[n]);
	}

	glPopMatrix();
}
*/