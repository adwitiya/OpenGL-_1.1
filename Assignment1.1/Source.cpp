
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "maths_funcs.h"

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#define _CRT_NONSTDC_NO_WARNINGS
// Two Transformation Matrix for 2 Triangles 
mat4 gEnvo = identity_mat4();
GLuint gEnvoID;
char keyFunction;

using namespace std;

// Note: Input to this shader is the vertex positions that we specified for the triangle. 
// Note: gl_Position is a special built-in variable that is supposed to contain the vertex position (in X, Y, Z, W)
// Since our triangle vertices were specified as vec3, we just set W to 1.0.


// Note: no input in this shader, it just outputs the colour of all fragments, in this case set to red (format: R, G, B, A).
// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
// Function to read shader code from external files
std::string readShaderSource(const std::string& fileName)
{
	std::ifstream file(fileName.c_str());
	if (file.fail()) {
		cout << "error loading shader called " << fileName;
		exit(1);
	}
	std::stringstream stream;
	stream << file.rdbuf();
	file.close();

	return stream.str();
}
static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	// Read shader data from file using readShaderSource function
	std::string outShader = readShaderSource(pShaderText);
	const char* pShaderSource = outShader.c_str();

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}


GLuint CompileShaders()
{
	
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	GLuint shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	// Vertex Shader 
	AddShader(shaderProgramID,"../Assignment1.1/simpleVertexShader.vert", GL_VERTEX_SHADER);
	// Fragment Shader
	AddShader(shaderProgramID,"../Assignment1.1/simpleFragmentShader.frag", GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };


	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
GLuint generateObjectBuffer(GLfloat vertices[], GLfloat colors[]) {
	GLuint numVertices = 3;
	// Genderate 1 generic buffer object, called VBO
	GLuint VBO;
	glGenBuffers(1, &VBO);
	// In OpenGL, we bind (make active) the handle to a target name and then execute commands on that target
	// Buffer will contain an array of vertices 
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// After binding, we now fill our object with data, everything in "Vertices" goes to the GPU
	glBufferData(GL_ARRAY_BUFFER, numVertices * 7 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	// if you have more data besides vertices (e.g., vertex colours or normals), use glBufferSubData to tell the buffer when the vertices array ends and when the colors start
	glBufferSubData(GL_ARRAY_BUFFER, 0, numVertices * 3 * sizeof(GLfloat), vertices);
	glBufferSubData(GL_ARRAY_BUFFER, numVertices * 3 * sizeof(GLfloat), numVertices * 4 * sizeof(GLfloat), colors);
	return VBO;
}

void linkCurrentBuffertoShader(GLuint shaderProgramID) {
	GLuint numVertices = 3;
	// find the location of the variables that we will be using in the shader program
	GLuint positionID = glGetAttribLocation(shaderProgramID, "vPosition");
	GLuint colorID = glGetAttribLocation(shaderProgramID, "vColor");
// Getting the fragment shader ID for our location matrix
	GLuint gEnvoID = glGetUniformLocation(shaderProgramID, "genvo");
	// Have to enable this
	glEnableVertexAttribArray(positionID);
	// Tell it where to find the position data in the currently active buffer (at index positionID)
	glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Similarly, for the color data.
	glEnableVertexAttribArray(colorID);
	glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numVertices * 3 * sizeof(GLfloat)));
}
#pragma endregion VBO_FUNCTIONS


void display() {

	glClear(GL_COLOR_BUFFER_BIT);
	// NB: Make the call to draw the geometry in the currently activated vertex buffer. This is where the GPU starts to work!
	glUniformMatrix4fv(gEnvoID, 1, GL_FALSE, gEnvo.m);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glutSwapBuffers();
}

// Method to handle keyboard calls
void keyPressed(unsigned char key, int x, int y) {
	
	switch (key)
	{
		// Translation
	case 't':
		keyFunction = 't';
		printf("translate activated \n");
		glutPostRedisplay();
		break;
		// Rotate
	case 'r':
		gEnvo = rotate_z_deg(gEnvo, 1);
		printf("rotate \n");
		glutPostRedisplay();
		break;
		// Uniform Scaling
	case 's':
		keyFunction = 's';
		printf("scale activated\n");
		break;
		// Non-uniform Scaling
	case 'n':
		printf("non-uniform \n");
		gEnvo = scale(gEnvo, vec3(0.91, 0.51, 0.31));
		glutPostRedisplay();
		break;
	case 'c':
		printf("combined transformation\n");
		gEnvo = translate(gEnvo, vec3(0.5, 0.5, 0.0));
		gEnvo = rotate_z_deg(gEnvo, 4);
		gEnvo = scale(gEnvo, vec3(0.01, 0.01, 0.01));
		glutPostRedisplay();
		break; 
	}
}

// Method to handle special keys function
void keySpecial(int keyspecial, int x, int y) {
	switch (keyFunction) {

	case 't':
	switch (keyspecial)
	{
	case GLUT_KEY_UP:
		gEnvo = translate(gEnvo, vec3(0.0, 0.5, 0.0));
		printf("moveup\n");
		glutPostRedisplay();
		break;
		
	case GLUT_KEY_DOWN:
		gEnvo = translate(gEnvo, vec3(0.0, -0.5, 0.0));
		printf("movedown\n");
		glutPostRedisplay();
		break;
	case GLUT_KEY_RIGHT:
		gEnvo = translate(gEnvo, vec3(0.5, 0.0, 0.0));
		printf("moveright\n");
		glutPostRedisplay();
		break;
	case GLUT_KEY_LEFT:
		gEnvo = translate(gEnvo, vec3(-0.5, 0.0, 0.0));
		printf("moveleft\n");
		glutPostRedisplay();
		break;
	}
	case 's':
	switch (keyspecial)
	{
	case GLUT_KEY_UP:
		gEnvo = scale(gEnvo, vec3(1.1, 1.1, 1.1));
		printf("scaleup\n");
		glutPostRedisplay();
		break;
	case GLUT_KEY_DOWN:
		gEnvo = scale(gEnvo, vec3(0.90, 0.90, 0.90));
		printf("scaledown\n");
		glutPostRedisplay();
		break;
	}
	}
}

void init()
{

	// Testing Git repo update 
	// Create 3 vertices that make up a triangle that fits on the viewport 
	GLfloat vertices[] = { 
		-0.3f, -0.3f, 0.0f,
		0.0f, 0.3f, 0.0f,
		0.3f, -0.3f, 0.0f };
	// Create a color array that identfies the colors of each vertex (format R, G, B, A)
	GLfloat colors[] = { 
		0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f, 
	};
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// Put the vertices and colors into a vertex buffer object
	generateObjectBuffer(vertices, colors);
	// Link the current buffer to the shader
	linkCurrentBuffertoShader(shaderProgramID);
	
}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Hello Transformation");
	// Tell glut where the display function is
	glutDisplayFunc(display);

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Function Call to interpret keyboard calls
	glutKeyboardFunc(keyPressed);
	glutSpecialFunc(keySpecial);
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}