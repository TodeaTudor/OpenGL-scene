//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"

#include <iostream>
#include "windows.h"
int glWindowWidth = 1920;
int glWindowHeight = 1080;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;
glm::vec3 spotLightPos;
GLuint spotLightPosLoc;
glm::vec3 spotLightDir;
GLuint modelPenguinLoc;
glm::mat4 modelPenguin;
GLuint modelPenguinLeftArmLoc;
glm::mat4 modelPenguinLeftArm;
GLuint modelPenguinRightArmLoc;
glm::mat4 modelPenguinRightArm;
GLuint modelWholePenguinLoc;
glm::mat4 modelWholePenguin;
GLuint spotLightDirLoc;
float cameraSpeed = 0.01f;
GLint objectShadow = 1;
bool pressedKeys[1024];
float angleY = 0.0f;
GLfloat lightAngle;
gps::Model3D dome;
gps::Model3D lightPole;
gps::Model3D ground;
gps::Model3D snowflake;
gps::Model3D screenQuad;
gps::Model3D wholePenguin;
gps::Model3D penguin;
gps::Model3D penguinLeftArm;
gps::Model3D penguinRightArm;
gps::Shader myCustomShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;
GLuint shadowMapFBO;
GLuint depthMapTexture;
GLuint shadowMapSpotFBO;
GLuint depthMapTextureSpot;
bool showDepthMap = false;
gps::Camera myCam = gps::Camera(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = 400, lastY = 300;
float pitch = -90.0f, yaw = -90.0f;
bool firstMouse = true;
float Zoom = 1.0f;

struct BoundingBox {
	glm::vec3 mins;
	glm::vec3 maxs;
};
void renderScene();
bool checkBBCollision(BoundingBox bb, glm::vec3 position);
BoundingBox computePenguinBB();
struct Snowflake {
	glm::vec3 startingPosition;
	glm::vec3 currentPosition;
};
Snowflake snowflakeArray[10000];

float windSpeed = 0.01f;
float yFall = 0.03f;
GLenum glCheckError_(const char *file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	myCustomShader.useShaderProgram();

	glm::mat4 projection = glm::perspective(glm::radians(myCam.Zoom), (float)retina_width / (float)retina_height, 0.1f, 100.0f);
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	myCam.rotate(pitch, yaw);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	myCam.zoom(yoffset);
}

void processMovement()
{
	if (pressedKeys[GLFW_KEY_3]) {
		glDisable(GL_MULTISAMPLE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	if (pressedKeys[GLFW_KEY_1]) {
		glDisable(GL_MULTISAMPLE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (pressedKeys[GLFW_KEY_2]) {
		glfwWindowHint(GLFW_SAMPLES, 4);
		glEnable(GL_MULTISAMPLE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (pressedKeys[GLFW_KEY_4]) {
		glm::vec3 initCameraPos = myCam.getCameraPosition();
		glm::vec3 camTarget = myCam.getCameraTarget();
		glm::vec3 nextPos = initCameraPos;
		std::vector<glm::vec3> path;
		path.push_back(glm::vec3(14.014576f, 6.671526f, 14.014576f));
		path.push_back(glm::vec3(-6.246339f, 6.671526f, 18.564970f));
		path.push_back(glm::vec3(-24.776463f, 10.385893f, -4.093956f));
		path.push_back(glm::vec3(18.391695f, 6.062449f, -15.704090f));
		path.push_back(glm::vec3(-4.693607f, 0.968584f, 3.044745f));
		std::vector<glm::vec3> cameraLocations;
		glm::vec3 prev_path = initCameraPos;
		for (int i = 0; i < path.size(); i++) {
			glm::vec3 distance = path[i] - prev_path;
			float x_inc = distance.x / 200.0f;
			float y_inc = distance.y / 200.0f;
			float z_inc = distance.z / 200.0f;
			for (int j = 0; j < 200; j++) {
				cameraLocations.push_back(glm::vec3(x_inc, y_inc, z_inc));
			}
			prev_path = path[i];
		}
		int animationIndex = 0;
		while (!pressedKeys[GLFW_KEY_5] && (animationIndex < cameraLocations.size() - 1)) {
			nextPos += cameraLocations[animationIndex];
			myCam = gps::Camera(nextPos, camTarget, glm::vec3(0.0f, 0.1f, 0.0f));
			renderScene();
			glfwPollEvents();
			glfwSwapBuffers(glWindow);
			animationIndex++;
		}
	}
	
	if (pressedKeys[GLFW_KEY_P]) {
		printf("%f, %f, %f\n", myCam.getCameraPosition().x, myCam.getCameraPosition().y, myCam.getCameraPosition().z);
	}


	if (pressedKeys[GLFW_KEY_J]) {
		lightAngle -= 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle += 1.0f;
	}


	if (pressedKeys[GLFW_KEY_UP]) {
		windSpeed += 0.01f;
	}

	if (pressedKeys[GLFW_KEY_DOWN]) {
		if (windSpeed > 0.01f)
			windSpeed -= 0.01f;
	}

	if (pressedKeys[GLFW_KEY_W]) {
		if (!checkBBCollision(computePenguinBB(), myCam.getCameraPosition() + myCam.getCameraFrontDirection() * cameraSpeed)) {
			myCam.move(gps::MOVE_FORWARD, cameraSpeed);
		}
			
	}

	if (pressedKeys[GLFW_KEY_S]) {
		if (!checkBBCollision(computePenguinBB(), myCam.getCameraPosition() - myCam.getCameraFrontDirection() * cameraSpeed)) {
			myCam.move(gps::MOVE_BACKWARD, cameraSpeed);
		}
	}

	if (pressedKeys[GLFW_KEY_A]) {
		if (!checkBBCollision(computePenguinBB(), myCam.getCameraPosition() - glm::normalize(glm::cross(myCam.getCameraFrontDirection(), glm::vec3(0.0f, 1.0f, 0.0f)) * cameraSpeed))) {
			myCam.move(gps::MOVE_LEFT, cameraSpeed);
		}
	}

	if (pressedKeys[GLFW_KEY_D]) {
		if (!checkBBCollision(computePenguinBB(), myCam.getCameraPosition() + glm::normalize(glm::cross(myCam.getCameraFrontDirection(), glm::vec3(0.0f,1.0f,0.0f)) * cameraSpeed))) {
			myCam.move(gps::MOVE_RIGHT, cameraSpeed);
		}
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouse_callback);
	glfwSetScrollCallback(glWindow, scroll_callback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3, 0.3, 0.3, 1.0);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glEnable(GL_FRAMEBUFFER_SRGB);
}

void initObjects() {
	dome.LoadModel("objects/skydome/skydome.obj");
	lightPole.LoadModel("objects/LightPole/pole.obj");
	ground.LoadModel("objects/snowy/final_scene.obj");
	snowflake.LoadModel("objects/snowflake/snowflake.obj");
	screenQuad.LoadModel("objects/quad/quad.obj");
	penguin.LoadModel("objects/penguin_body/pengu.obj");
	penguinRightArm.LoadModel("objects/penguin_body/right_arm.obj");
	penguinLeftArm.LoadModel("objects/penguin_body/left_arm.obj");
	wholePenguin.LoadModel("objects/penguin/penguin.obj");

}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	depthMapShader.loadShader("shaders/depth.vert", "shaders/depth.frag");
	depthMapShader.useShaderProgram();
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();

}

void initUniforms() {
	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	modelPenguin = glm::mat4(1.0f);
	modelPenguinLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelPenguinLoc, 1, GL_FALSE, glm::value_ptr(modelPenguin));

	modelWholePenguin = glm::mat4(1.0f);
	modelWholePenguinLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelWholePenguinLoc, 1, GL_FALSE, glm::value_ptr(modelWholePenguin));

	modelPenguinRightArm = glm::mat4(1.0f);
	modelPenguinRightArmLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelPenguinLoc, 1, GL_FALSE, glm::value_ptr(modelPenguinRightArm));
	
	modelPenguinLeftArm = glm::mat4(1.0f);
	modelPenguinLeftArmLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelPenguinLoc, 1, GL_FALSE, glm::value_ptr(modelPenguinLeftArm));

	view = myCam.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	
	glm::mat4 projection = glm::perspective(glm::radians(myCam.Zoom), 800.0f / 600.0f, 0.1f, 200.0f);
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 15.0f, 10.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");	
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	spotLightPos = glm::vec3(0.3f, 1.3f, 0.0f);
	spotLightPosLoc = glGetUniformLocation(myCustomShader.shaderProgram, "spotLightPos");
	glUniform3fv(spotLightPosLoc, 1, glm::value_ptr(spotLightPos));

	spotLightDir = glm::vec3(1.0f, 0.0f, 0.0f);
	spotLightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "spotLightDir");
	glUniform3fv(spotLightDirLoc, 1, glm::value_ptr(spotLightDir));
	
}

void initFBO() {
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);
	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,
		0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
}

glm::mat4 computeLightSpaceTrMatrix() {
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightView = glm::lookAt(glm::mat3(lightRotation) * lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const GLfloat near_plane = 0.1f, far_plane = 40.0f;
	glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

	return lightSpaceTrMatrix;
}

std::vector<glm::vec3> penguinPath;
void computePenguinPath() {
	
	for (int i = 0; i < 3600; i++) {
		penguinPath.push_back(glm::vec3(-2.0f, -0.45f, 0.002f * i));
	}
}


int penguinIndex = 1;
int penguinMovementIndex = 0;
int foot = 0;
void drawPenguin(gps::Shader shader, bool depthPass) {
	modelWholePenguin = glm::mat4(1.0f);
	modelWholePenguin = glm::translate(modelWholePenguin, glm::vec3(penguinPath[penguinIndex].x, penguinPath[penguinIndex].y, penguinPath[penguinIndex].z));
	
	if (penguinIndex < 3599) {		
		penguinMovementIndex++;
		if (penguinMovementIndex < 40) {
			if (foot == 0) {
				modelWholePenguin = glm::rotate(modelWholePenguin, glm::radians(0.12f * penguinMovementIndex), glm::vec3(0.0f, 0.0f, 1.0f));
			}
			else {
				modelWholePenguin = glm::rotate(modelWholePenguin, glm::radians(-0.12f * penguinMovementIndex), glm::vec3(0.0f, 0.0f, 1.0f));
				modelWholePenguin = glm::rotate(modelWholePenguin, glm::radians(-24.0f), glm::vec3(0.0f, 1.0f, 0.0f));

			}
		}
		if (penguinMovementIndex < 80 && penguinMovementIndex >= 40) {
			if (foot == 0) {
				modelWholePenguin = glm::rotate(modelWholePenguin, glm::radians(4.8f), glm::vec3(0.0f, 0.0f, 1.0f));
				modelWholePenguin = glm::rotate(modelWholePenguin, glm::radians(-0.6f * (penguinMovementIndex - 40)), glm::vec3(0.0f, 1.0f, 0.0f));

			}
			else {
				modelWholePenguin = glm::rotate(modelWholePenguin, glm::radians(-4.8f), glm::vec3(0.0f, 0.0f, 1.0f));
				modelWholePenguin = glm::rotate(modelWholePenguin, glm::radians(-24.0f + 0.6f * (penguinMovementIndex - 40)), glm::vec3(0.0f, 1.0f, 0.0f));
			}

			if (penguinMovementIndex % 5 == 0) {
				penguinIndex++;
			}
		}


		if (penguinMovementIndex < 120 && penguinMovementIndex >= 80) {
			if (foot == 0) {
				modelWholePenguin = glm::rotate(modelWholePenguin, glm::radians(4.8f - 0.12f * (penguinMovementIndex - 80)), glm::vec3(0.0f, 0.0f, 1.0f));
				modelWholePenguin = glm::rotate(modelWholePenguin, glm::radians(-24.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			}
			else {
				modelWholePenguin = glm::rotate(modelWholePenguin, glm::radians(-4.8f + 0.12f * (penguinMovementIndex - 80)), glm::vec3(0.0f, 0.0f, 1.0f));

			}

		}


		if (penguinMovementIndex == 119) {
			penguinMovementIndex = 0;
			if (foot == 0) {
				foot = 1;
			}
			else {
				foot = 0;
			}
		}
	}
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelWholePenguin));
	
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelWholePenguin));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	wholePenguin.Draw(shader);

}


BoundingBox computePenguinBB() {
	float maxX = -FLT_MAX;
	float minX = FLT_MAX;
	float maxZ = -FLT_MAX;
	float minZ = FLT_MAX;
	float maxY = -FLT_MAX;
	float minY = FLT_MAX;
	for (gps::Mesh mesh : wholePenguin.meshes) {
		for (gps::Vertex vertex : mesh.vertices) {
			if (vertex.Position.x > maxX) {
				maxX = vertex.Position.x;
			}
			if (vertex.Position.x < minX) {
				minX = vertex.Position.x;
			}
			if (vertex.Position.y > maxY) {
				maxY = vertex.Position.y;
			}
			if (vertex.Position.y < maxY) {
				minY = vertex.Position.y;
			}
			if (vertex.Position.z > maxZ) {
				maxZ = vertex.Position.z;
			}
			if (vertex.Position.z < minZ) {
				minZ = vertex.Position.z;
			}
		}
	}
	BoundingBox bb;
	bb.maxs = glm::vec3(penguinPath[penguinIndex].x + maxX, penguinPath[penguinIndex].y + maxY + 0.52f, penguinPath[penguinIndex].z + maxZ);
	bb.mins = glm::vec3(penguinPath[penguinIndex].x + minX, penguinPath[penguinIndex].y + minY - 0.52f, penguinPath[penguinIndex].z + minZ);
	return bb;
}

bool checkBBCollision(BoundingBox bb, glm::vec3 position) {
	bool xCol = false;
	bool yCol = false;
	bool zCol = false;

	if (position.x > bb.mins.x && position.x < bb.maxs.x) {
		xCol = true;
	}
	if (position.y > bb.mins.y && position.y < bb.maxs.y) {
		yCol = true;
	}
	if (position.z > bb.mins.z && position.z < bb.maxs.z) {
		zCol = true;
	}

	return (xCol && yCol && zCol);
}

int raiseWings = 1;
float wingsAngle = 0.0f;
void drawFlappingPenguin(gps::Shader shader, bool depthPass) {
	modelPenguinRightArm = glm::mat4(1.0f);
	modelPenguinLeftArm = glm::mat4(1.0f);
	
	modelPenguin = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.45f, -3.0f));
	
	modelPenguinLeftArm = glm::translate(modelPenguinLeftArm, glm::vec3(0.18826f, -0.45f + 0.66209f, -3.0f + 0.012794f));
	modelPenguinLeftArm = glm::rotate(modelPenguinLeftArm, glm::radians(wingsAngle), glm::vec3(0.0f, 0.0f, 1.0f));

	modelPenguinRightArm = glm::translate(modelPenguinRightArm, glm::vec3(-0.18826f, -0.45f + 0.66209f, -3.0f + 0.012794f));
	modelPenguinRightArm = glm::rotate(modelPenguinRightArm, glm::radians(-wingsAngle), glm::vec3(0.0f, 0.0f, 1.0f));
	
	if (wingsAngle == 110) {
		raiseWings = 0;
	}
	else if (wingsAngle == 0) {
		raiseWings = 1;
	}
	
	if (raiseWings == 0) {
		wingsAngle -= 0.5f;

	}
	else {
		wingsAngle += 0.5f;
	}

	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelPenguin));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelPenguin));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	penguin.Draw(shader);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelPenguinLeftArm));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelPenguinLeftArm));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	penguinLeftArm.Draw(shader);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelPenguinRightArm));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelPenguinRightArm));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	penguinRightArm.Draw(shader);
}


void drawObjects(gps::Shader shader, bool depthPass) {

	shader.useShaderProgram();
	glUniform1i(glGetUniformLocation(shader.shaderProgram, "objectShadow"), true);

	BoundingBox penguinBB = computePenguinBB();
	drawPenguin(shader, depthPass);
	drawFlappingPenguin(shader, depthPass);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.6f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}



	model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));
	model = glm::scale(model, glm::vec3(0.01f));


	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	lightPole.Draw(shader);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.2f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}


	ground.Draw(shader);
	glUniform1i(glGetUniformLocation(shader.shaderProgram, "objectShadow"), false);
	if (!depthPass) {
		dome.Draw(shader);
	}


	if (!depthPass) {
		for (int i = 0; i < 10000; i++) {
			if (!checkBBCollision(penguinBB, glm::vec3(snowflakeArray[i].currentPosition.x + windSpeed, snowflakeArray[i].currentPosition.y - yFall, snowflakeArray[i].currentPosition.z))) {
				model = glm::translate(glm::mat4(1.0f), glm::vec3(snowflakeArray[i].currentPosition.x + windSpeed, snowflakeArray[i].currentPosition.y - yFall, snowflakeArray[i].currentPosition.z));
				model = glm::scale(model, glm::vec3(0.5f));
				glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

				normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
				glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

				snowflake.Draw(shader);
				if (snowflakeArray[i].currentPosition.x > 61.0f || snowflakeArray[i].currentPosition.x < -61.0f) {
					snowflakeArray[i].currentPosition.x = snowflakeArray[i].startingPosition.x;
				}
				else {
					snowflakeArray[i].currentPosition.x = snowflakeArray[i].currentPosition.x + windSpeed;
				}
				if (snowflakeArray[i].currentPosition.y < -1.0f) {
					snowflakeArray[i].currentPosition.y = 70.0f;
				}
				else {
					snowflakeArray[i].currentPosition.y = snowflakeArray[i].currentPosition.y - yFall;
				}

			}
			else {
				snowflakeArray[i].currentPosition.y = 70.0f;
			}

		}
	}
}

void renderScene() {

	
	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
	
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawObjects(depthMapShader, true);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	cameraSpeed = 10.0f * deltaTime;
	if (showDepthMap) {
		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT);

		screenQuadShader.useShaderProgram();

		//bind the depth map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);

	}
	else {


		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		myCustomShader.useShaderProgram();

		view = myCam.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
				
		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
	
		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(computeLightSpaceTrMatrix()));


		glm::mat4 projection = glm::perspective(glm::radians(myCam.Zoom), 800.0f / 600.0f, 0.1f, 200.0f);
		GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		drawObjects(myCustomShader, false);

	}
}

void cleanup() {
	glDeleteTextures(1,& depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();
}

int main(int argc, const char * argv[]) {

	int positioningIndex = 0;
	for (int i = 0; i < 10000; i++) {
		snowflakeArray[i].startingPosition.y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 70.0f));
		if (positioningIndex == 0) {
			snowflakeArray[i].startingPosition.x = -static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 60.0f));
			snowflakeArray[i].startingPosition.z = -static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 60.0f));
		}
		else if (positioningIndex == 1) {
			snowflakeArray[i].startingPosition.x = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 60.0f));
			snowflakeArray[i].startingPosition.z = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 60.0f));
		}
		else if (positioningIndex == 2) {
			snowflakeArray[i].startingPosition.x = -static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 60.0f));
			snowflakeArray[i].startingPosition.z = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 60.0f));
		}
		else if (positioningIndex == 3) {
			snowflakeArray[i].startingPosition.x = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 60.0f));
			snowflakeArray[i].startingPosition.z = -static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 60.0f));
		}
		snowflakeArray[i].currentPosition = snowflakeArray[i].startingPosition;
		if (positioningIndex == 3) {
			positioningIndex = 0;
		}
		else {
			positioningIndex++;
		}
	}

	computePenguinPath();
	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}

	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms();
	initFBO();
	
	glCheckError();

	while (!glfwWindowShouldClose(glWindow)) {
		
		processMovement();
		renderScene();		

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	cleanup();

	return 0;
}
