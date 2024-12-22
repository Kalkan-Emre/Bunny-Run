#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <GL/glew.h>   // The GL Header File
#include <GL/gl.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

GLuint gProgram[6];
GLint gIntensityLoc;
float gIntensity = 1000;
int gWidth = 640, gHeight = 480;

GLint modelingMatrixLoc[6];
GLint viewingMatrixLoc[6];
GLint projectionMatrixLoc[6];
GLint eyePosLoc[6];

glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix;
glm::vec3 eyePos(0, 0, 0);

int score = 0;

struct Vertex
{
    Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Texture
{
    Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) { }
    GLfloat u, v;
};

struct Normal
{
    Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Face
{
	Face(int v[], int t[], int n[]) {
		vIndex[0] = v[0];
		vIndex[1] = v[1];
		vIndex[2] = v[2];
		tIndex[0] = t[0];
		tIndex[1] = t[1];
		tIndex[2] = t[2];
		nIndex[0] = n[0];
		nIndex[1] = n[1];
		nIndex[2] = n[2];
	}
    GLuint vIndex[3], tIndex[3], nIndex[3];
};

vector<Vertex> gVertices;
vector<Texture> gTextures;
vector<Normal> gNormals;
vector<Face> gFaces;

GLuint gVertexAttribBuffer, gTextVBO, gIndexBuffer;
GLint gInVertexLoc, gInNormalLoc;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

int aKeyPressed = false;
bool dKeyPressed = false;

struct Model 
{
	vector<Vertex> gVertices;
    vector<Texture> gTextures;
    vector<Normal> gNormals;
    vector<Face> gFaces;
    GLuint VAO, vertexBuffer, indexBuffer;
};

Model bunny, quad, cube;

float bunnySpeed = 1.5; 
float horizontalSpeed = 1.5;
float move_bunny_x = 0.0;
float move_bunny_z = -3.5;
float bunny_height = 0.0;

float checkpointPosition = -100.0;

int checkpointIsYellow[3];
int yellowCheckpointIndex = rand() % 3;


glm::vec3 checkpointPositions[3];
bool checkpointHit[3] = {false, false, false};

bool pause = false;
bool restart = false;
bool happy_state = false;
bool hit_yellow = false;


int hit = -1;
bool hit_current_round = false;

float maxSpeed = 22.0;
float maxHorizontalSpeed = 22.0;

glm::vec3 text_color(1.0, 1.0, 1.0);

static float angle = -90;
static float happy_state_angle = 0;
float angleRad = (float)(angle / 180.0) * M_PI;
float happy_state_rad;


bool ParseObj(Model& model,const string& fileName)
{
	fstream myfile;

	// Open the input 
	myfile.open(fileName.c_str(), std::ios::in);

	if (myfile.is_open())
	{
		string curLine;

		while (getline(myfile, curLine))
		{
			stringstream str(curLine);
			GLfloat c1, c2, c3;
			GLuint index[9];
			string tmp;

			if (curLine.length() >= 2)
			{
				if (curLine[0] == 'v')
				{
					if (curLine[1] == 't') // texture
					{
						str >> tmp; // consume "vt"
						str >> c1 >> c2;
						model.gTextures.push_back(Texture(c1, c2));
					}
					else if (curLine[1] == 'n') // normal
					{
						str >> tmp; // consume "vn"
						str >> c1 >> c2 >> c3;
						model.gNormals.push_back(Normal(c1, c2, c3));
					}
					else // vertex
					{
						str >> tmp; // consume "v"
						str >> c1 >> c2 >> c3;
						model.gVertices.push_back(Vertex(c1, c2, c3));
					}
				}
				else if (curLine[0] == 'f') // face
				{
					str >> tmp; // consume "f"
					char c;
					int vIndex[3], nIndex[3], tIndex[3];
					str >> vIndex[0]; str >> c >> c; // consume "//"
					str >> nIndex[0];
					str >> vIndex[1]; str >> c >> c; // consume "//"
					str >> nIndex[1];
					str >> vIndex[2]; str >> c >> c; // consume "//"
					str >> nIndex[2];

					assert(vIndex[0] == nIndex[0] &&
						vIndex[1] == nIndex[1] &&
						vIndex[2] == nIndex[2]); // a limitation for now

					// make indices start from 0
					for (int c = 0; c < 3; ++c)
					{
						vIndex[c] -= 1;
						nIndex[c] -= 1;
						tIndex[c] -= 1;
					}

					model.gFaces.push_back(Face(vIndex, tIndex, nIndex));
				}
				else
				{
					cout << "Ignoring unidentified line in obj file: " << curLine << endl;
				}
			}

			//data += curLine;
			if (!myfile.eof())
			{
				//data += "\n";
			}
		}

		myfile.close();
	}
	else
	{
		return false;
	}

	assert(model.gVertices.size() == model.gNormals.size());

	return true;
}

bool ReadDataFromFile(
	const string& fileName, ///< [in]  Name of the shader file
	string& data)     ///< [out] The contents of the file
{
	fstream myfile;

	// Open the input 
	myfile.open(fileName.c_str(), std::ios::in);

	if (myfile.is_open())
	{
		string curLine;

		while (getline(myfile, curLine))
		{
			data += curLine;
			if (!myfile.eof())
			{
				data += "\n";
			}
		}

		myfile.close();
	}
	else
	{
		return false;
	}

	return true;
}

void createVS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = {0};
    glGetShaderInfoLog(vs, 1024, &length, output);
    printf("VS compile log: %s\n", output);

    glAttachShader(program, vs);
}

void createFS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader, &length);
    glCompileShader(fs);

    char output[1024] = {0};
    glGetShaderInfoLog(fs, 1024, &length, output);
    printf("FS compile log: %s\n", output);

    glAttachShader(program, fs);
}

GLuint createVS(const char* shaderName)
{
	string shaderSource;

	string filename(shaderName);
	if (!ReadDataFromFile(filename, shaderSource))
	{
		cout << "Cannot find file name: " + filename << endl;
		exit(-1);
	}

	GLint length = shaderSource.length();
	const GLchar* shader = (const GLchar*)shaderSource.c_str();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &shader, &length);
	glCompileShader(vs);

	char output[1024] = { 0 };
	glGetShaderInfoLog(vs, 1024, &length, output);
	printf("VS compile log: %s\n", output);

	return vs;
}

GLuint createFS(const char* shaderName)
{
	string shaderSource;

	string filename(shaderName);
	if (!ReadDataFromFile(filename, shaderSource))
	{
		cout << "Cannot find file name: " + filename << endl;
		exit(-1);
	}

	GLint length = shaderSource.length();
	const GLchar* shader = (const GLchar*)shaderSource.c_str();

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &shader, &length);
	glCompileShader(fs);

	char output[1024] = { 0 };
	glGetShaderInfoLog(fs, 1024, &length, output);
	printf("FS compile log: %s\n", output);

	return fs;
}

void initShaders()
{
    gProgram[0] = glCreateProgram();
    gProgram[1] = glCreateProgram();
    gProgram[2] = glCreateProgram();//Text
    gProgram[3] = glCreateProgram();//Bunny
    gProgram[4] = glCreateProgram();//Ground
    gProgram[5] = glCreateProgram();//Cube

    createVS(gProgram[0], "vert0.glsl");
    createFS(gProgram[0], "frag0.glsl");

    createVS(gProgram[1], "vert1.glsl");
    createFS(gProgram[1], "frag1.glsl");

    createVS(gProgram[2], "vert_text.glsl");
    createFS(gProgram[2], "frag_text.glsl");

    GLuint vert_bunny = createVS("vert_bunny.glsl");
	GLuint frag_bunny = createFS("frag_bunny.glsl");

	GLuint vert_ground = createVS("vert_ground.glsl");
	GLuint frag_ground = createFS("frag_ground.glsl");
	
	GLuint vert_cube = createVS("vert_cp.glsl");
	GLuint frag_cube = createFS("frag_cp.glsl");

    glAttachShader(gProgram[3], vert_bunny);
	glAttachShader(gProgram[3], frag_bunny);

	glAttachShader(gProgram[4], vert_ground);
	glAttachShader(gProgram[4], frag_ground);

	glAttachShader(gProgram[5], vert_cube);
	glAttachShader(gProgram[5], frag_cube);

	glLinkProgram(gProgram[3]);
	GLint status;
	glGetProgramiv(gProgram[3], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	glLinkProgram(gProgram[4]);
	glGetProgramiv(gProgram[4], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	glLinkProgram(gProgram[5]);
	glGetProgramiv(gProgram[5], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	for (int i = 3; i < 6; ++i)
	{
		modelingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "modelingMatrix");
		viewingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "viewingMatrix");
		projectionMatrixLoc[i] = glGetUniformLocation(gProgram[i], "projectionMatrix");
		eyePosLoc[i] = glGetUniformLocation(gProgram[i], "eyePos");
	}

    glBindAttribLocation(gProgram[0], 0, "inVertex");
    glBindAttribLocation(gProgram[0], 1, "inNormal");
    glBindAttribLocation(gProgram[1], 0, "inVertex");
    glBindAttribLocation(gProgram[1], 1, "inNormal");
    glBindAttribLocation(gProgram[2], 2, "vertex");

    glLinkProgram(gProgram[0]);
    glLinkProgram(gProgram[1]);
    glLinkProgram(gProgram[2]);
    glUseProgram(gProgram[0]);

    gIntensityLoc = glGetUniformLocation(gProgram[0], "intensity");
    cout << "gIntensityLoc = " << gIntensityLoc << endl;
    glUniform1f(gIntensityLoc, gIntensity);
}

void initVBO(Model& model)
{
	glGenVertexArrays(1, &(model.VAO));
	assert(model.VAO > 0);
	glBindVertexArray(model.VAO);
	cout << "vao = " << model.VAO << endl;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	assert(glGetError() == GL_NONE);

    glGenBuffers(1, &(model.vertexBuffer));
    glGenBuffers(1, &(model.indexBuffer));
	cout << "vertexBuffer = " << model.vertexBuffer << endl;
	cout << "indexBuffer = " << model.indexBuffer << endl;

	assert(model.vertexBuffer > 0 && model.indexBuffer > 0);

	glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indexBuffer);

	gVertexDataSizeInBytes = model.gVertices.size() * 3 * sizeof(GLfloat);
	gNormalDataSizeInBytes = model.gNormals.size() * 3 * sizeof(GLfloat);
	int indexDataSizeInBytes = model.gFaces.size() * 3 * sizeof(GLuint);
	GLfloat* vertexData = new GLfloat[model.gVertices.size() * 3];
	GLfloat* normalData = new GLfloat[model.gNormals.size() * 3];
	GLuint* indexData = new GLuint[model.gFaces.size() * 3];

	float minX = 1e6, maxX = -1e6;
	float minY = 1e6, maxY = -1e6;
	float minZ = 1e6, maxZ = -1e6;

	for (int i = 0; i < model.gVertices.size(); ++i)
	{
		vertexData[3 * i] = model.gVertices[i].x;
		vertexData[3 * i + 1] = model.gVertices[i].y;
		vertexData[3 * i + 2] = model.gVertices[i].z;

		minX = std::min(minX, model.gVertices[i].x);
		maxX = std::max(maxX, model.gVertices[i].x);
		minY = std::min(minY, model.gVertices[i].y);
		maxY = std::max(maxY, model.gVertices[i].y);
		minZ = std::min(minZ, model.gVertices[i].z);
		maxZ = std::max(maxZ, model.gVertices[i].z);
	}

	std::cout << "minX = " << minX << std::endl;
	std::cout << "maxX = " << maxX << std::endl;
	std::cout << "minY = " << minY << std::endl;
	std::cout << "maxY = " << maxY << std::endl;
	std::cout << "minZ = " << minZ << std::endl;
	std::cout << "maxZ = " << maxZ << std::endl;

	for (int i = 0; i < model.gNormals.size(); ++i)
	{
		normalData[3 * i] = model.gNormals[i].x;
		normalData[3 * i + 1] = model.gNormals[i].y;
		normalData[3 * i + 2] = model.gNormals[i].z;
	}

	for (int i = 0; i < model.gFaces.size(); ++i)
	{
		indexData[3 * i] = model.gFaces[i].vIndex[0];
		indexData[3 * i + 1] = model.gFaces[i].vIndex[1];
		indexData[3 * i + 2] = model.gFaces[i].vIndex[2];
	}


	glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexData);
	glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, normalData);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);

	// done copying to GPU memory; can free now from CPU memory
	delete[] vertexData;
	delete[] normalData;
	delete[] indexData;

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));
	
	glBindVertexArray(0);
}

void initFonts(int windowWidth, int windowHeight)
{
    // Set OpenGL options
    //glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
    glUseProgram(gProgram[2]);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationSerif-Italic.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
                );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    // Configure VBO for texture quads
    //
    glGenBuffers(1, &gTextVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void init()
{
	checkpointIsYellow[0] = (0 == yellowCheckpointIndex);
	checkpointIsYellow[1] = (1 == yellowCheckpointIndex);
	checkpointIsYellow[2] = (2 == yellowCheckpointIndex);

	checkpointPositions[0] = glm::vec3(-6.0f, -3.0f, checkpointPosition);
	checkpointPositions[1] = glm::vec3(0.0f, -3.0f, checkpointPosition);
	checkpointPositions[2] = glm::vec3(6.0f, -3.0f, checkpointPosition);

	srand(time(NULL));
	glEnable(GL_DEPTH_TEST);
	initShaders();
	
	ParseObj(bunny, "bunny.obj");
	initVBO(bunny);
	ParseObj(quad, "quad.obj");
	initVBO(quad);
	ParseObj(cube, "cube.obj");
	initVBO(cube);
	initFonts(gWidth, gHeight);
	
}

void drawModel()
{
	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));

	glDrawElements(GL_TRIANGLES, gFaces.size() * 3, GL_UNSIGNED_INT, 0);
}

void updateBunnyPosition() {
    bunnySpeed += (0.05f*(1-bunnySpeed/maxSpeed)) * 0.016f;
    
    float hopHeight = 0.4 * sin(bunnySpeed * 5 * glfwGetTime());

    move_bunny_z -= bunnySpeed * 0.5f;
    bunny_height = -2.5f + hopHeight;
    
    if (aKeyPressed && move_bunny_x>-8) {
        move_bunny_x -= horizontalSpeed * 0.1f;
    }
    if (dKeyPressed && move_bunny_x<8) {
        move_bunny_x += horizontalSpeed * 0.1f;
    }
    
    horizontalSpeed += (0.05f*(1-horizontalSpeed/maxHorizontalSpeed)) * 0.016f;
}

int checkCollision(){
	glm::vec2 bunnyPos(move_bunny_x,move_bunny_z);

	if((move_bunny_z-1.238)<checkpointPosition){
		for(int i = 0; i < 3; i++){
			if((move_bunny_x<(checkpointPositions[i].x+1.45))&&(move_bunny_x>(checkpointPositions[i].x-1.45))){
				if(checkpointIsYellow[i]){
					//Handle score
					cout<<"Yellow checkpoint\n";
					score +=1000;
					happy_state = true;
					hit_yellow = true;

				}
				else{
					cout<<"Red checkpoint\n";
				}
				hit_current_round = true;
				return i;
			}

		}
	}
	return -1;
}

void renderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state	
    glUseProgram(gProgram[2]);
    glUniform3f(glGetUniformLocation(gProgram[2], "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },            
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }           
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        //glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)

        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}


void display()
{
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    updateBunnyPosition();
	if(!hit_current_round){
		hit = checkCollision();
	}

	glm::mat4 matT = glm::translate(glm::mat4(1.0), glm::vec3(move_bunny_x, bunny_height, move_bunny_z));
	glm::mat4 matS = glm::scale(glm::mat4(1.0), glm::vec3(0.5, 0.5, 0.5));
	glm::mat4 matRy = glm::rotate(glm::mat4(1.0), angleRad, glm::vec3(0.0, 1.0, 0.0));
	modelingMatrix = matT * matS * matRy;

	if(checkpointIsYellow[hit]==false && hit_current_round){
		glm::mat4 matRz = glm::rotate(glm::mat4(1.0), -angleRad, glm::vec3(-1.0, 0.0, 0.0));
		modelingMatrix = modelingMatrix * matRz;
		pause = true;
	}

	if(happy_state){
		happy_state_rad = (float)(happy_state_angle / 180.0) * M_PI;
		happy_state_angle -=  bunnySpeed * 10; 
		if(happy_state_angle<-360){
			happy_state_angle = 0;
			happy_state = false;
		}
		glm::mat4 matRy = glm::rotate(glm::mat4(1.0), happy_state_rad, glm::vec3(0.0, 1.0, 0.0));
		modelingMatrix = modelingMatrix * matRy;
	}

	matT = glm::translate(glm::mat4(1.0), glm::vec3(0, 0, (bunnySpeed * 0.5f)));
	viewingMatrix = viewingMatrix * matT;


	glBindVertexArray(bunny.VAO);
	glUseProgram(gProgram[3]);
	GLuint lightOffsetLoc = glGetUniformLocation(gProgram[3], "lightOffset");
	glUniform1f(lightOffsetLoc, move_bunny_z);
	glUniformMatrix4fv(projectionMatrixLoc[3], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(viewingMatrixLoc[3], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
	glUniformMatrix4fv(modelingMatrixLoc[3], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
	glUniform3fv(eyePosLoc[3], 1, glm::value_ptr(eyePos));
	glDrawElements(GL_TRIANGLES, bunny.gFaces.size() * 3, GL_UNSIGNED_INT, 0);


	glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 80.0f, 1000.0f));
	glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -3.0f, move_bunny_z));
	modelingMatrix = translate * scale * rotate;

	glBindVertexArray(quad.VAO);
	glUseProgram(gProgram[4]);
	GLuint offsetLoc = glGetUniformLocation(gProgram[4], "offset");
	GLuint scaleLoc = glGetUniformLocation(gProgram[4], "scale");
	glUniform1f(offsetLoc, -150.0);
	glUniform1f(scaleLoc, 0.2f);
	glUniformMatrix4fv(projectionMatrixLoc[4], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(viewingMatrixLoc[4], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
	glUniformMatrix4fv(modelingMatrixLoc[4], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
	glUniform3fv(eyePosLoc[4], 1, glm::value_ptr(eyePos));
	glDrawElements(GL_TRIANGLES, quad.gFaces.size() * 3, GL_UNSIGNED_INT, 0);

	if(checkpointPosition>move_bunny_z){
		checkpointPosition= move_bunny_z-110;
		hit = -1;
		hit_current_round = false;
		restart=false;
		yellowCheckpointIndex = rand() % 3;
		checkpointIsYellow[0] = (0 == yellowCheckpointIndex);
		checkpointIsYellow[1] = (1 == yellowCheckpointIndex);
		checkpointIsYellow[2] = (2 == yellowCheckpointIndex);
		checkpointPositions[0] = glm::vec3(-6.0f, -3.0f, checkpointPosition);
		checkpointPositions[1] = glm::vec3(0.0f, -3.0f, checkpointPosition);
		checkpointPositions[2] = glm::vec3(6.0f, -3.0f, checkpointPosition);
	}
	scale = glm::scale(glm::mat4(1.0f), glm::vec3(1.4f, 4.0f, 2.0f));
	translate = glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f, -3.0f, checkpointPosition));
	modelingMatrix = translate * scale;

	if(!(pause && hit==0)||(happy_state&&checkpointIsYellow[0]))
	{
		glBindVertexArray(cube.VAO);
		glUseProgram(gProgram[5]);
		GLuint lightOffsetLocCube1 = glGetUniformLocation(gProgram[5], "lightOffset");
		GLuint isYellowLocCube1 = glGetUniformLocation(gProgram[5], "isYellow");
		glUniform1f(lightOffsetLocCube1, move_bunny_z);
		glUniform1i(isYellowLocCube1, checkpointIsYellow[0]);
		glUniformMatrix4fv(projectionMatrixLoc[5], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
		glUniformMatrix4fv(viewingMatrixLoc[5], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
		glUniformMatrix4fv(modelingMatrixLoc[5], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
		glUniform3fv(eyePosLoc[5], 1, glm::value_ptr(eyePos));
		glDrawElements(GL_TRIANGLES, cube.gFaces.size() * 3, GL_UNSIGNED_INT, 0);
	}

	translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -3.0f, checkpointPosition));
	modelingMatrix = translate * scale;

	if(!(pause && hit==1)||(happy_state&&checkpointIsYellow[1]))
	{
		glBindVertexArray(cube.VAO);
		glUseProgram(gProgram[5]);
		GLuint isYellowLocCube2 = glGetUniformLocation(gProgram[5], "isYellow");
		GLuint lightOffsetLocCube2 = glGetUniformLocation(gProgram[5], "lightOffset");
		glUniform1f(lightOffsetLocCube2, move_bunny_z);
		glUniform1i(isYellowLocCube2, checkpointIsYellow[1]);
		glUniformMatrix4fv(projectionMatrixLoc[5], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
		glUniformMatrix4fv(viewingMatrixLoc[5], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
		glUniformMatrix4fv(modelingMatrixLoc[5], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
		glUniform3fv(eyePosLoc[5], 1, glm::value_ptr(eyePos));
		glDrawElements(GL_TRIANGLES, cube.gFaces.size() * 3, GL_UNSIGNED_INT, 0);
	}

	translate = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, -3.0f, checkpointPosition));
	modelingMatrix = translate * scale;

	if(!(pause && hit==2)||(happy_state&&checkpointIsYellow[5]))
	{
		glBindVertexArray(cube.VAO);
		glUseProgram(gProgram[5]);
		GLuint isYellowLocCube3 = glGetUniformLocation(gProgram[5], "isYellow");
		GLuint lightOffsetLocCube3 = glGetUniformLocation(gProgram[5], "lightOffset");
		glUniform1f(lightOffsetLocCube3, move_bunny_z);
		glUniform1i(isYellowLocCube3, checkpointIsYellow[2]);
		glUniformMatrix4fv(projectionMatrixLoc[5], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
		glUniformMatrix4fv(viewingMatrixLoc[5], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
		glUniformMatrix4fv(modelingMatrixLoc[5], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
		glUniform3fv(eyePosLoc[5], 1, glm::value_ptr(eyePos));
		glDrawElements(GL_TRIANGLES, cube.gFaces.size() * 3, GL_UNSIGNED_INT, 0);
	}
    //glBindBuffer(GL_ARRAY_BUFFER, 0);	
    glBindVertexArray(0);
    if(pause){
        renderText("Score:"+std::to_string(score), 10, 430, 0.7, glm::vec3(0.7, 0.0, 0.0));
    }
    else{
        renderText("Score:"+std::to_string(score), 10, 430, 0.7, glm::vec3(0.85, 0.8, 0.1));
    }

    assert(glGetError() == GL_NO_ERROR);

	angle += 0.5;
}


void reshape(GLFWwindow* window, int w, int h)
{
	w = w < 1 ? 1 : w;
	h = h < 1 ? 1 : h;

	gWidth = w;
	gHeight = h;

	glViewport(0, 0, w, h);

	// Use perspective projection
	float fovyRad = (float)(90.0 / 180.0) * M_PI;
	projectionMatrix = glm::perspective(fovyRad, w / (float)h, 1.0f, 100.0f);

	// Assume default camera position and orientation (camera is at
	// (0, 0, 0) with looking at -z direction and its up vector pointing
	// at +y direction)
	// 
	//viewingMatrix = glm::mat4(1);
	viewingMatrix = glm::lookAt(glm::vec3(0, 1, 2), glm::vec3(0, 0, 0) + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));

}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		srand(time(NULL));
		pause=false;
		restart = true;
		hit_current_round = false;
		score = 0;
		bunnySpeed = 1.5; 
		horizontalSpeed = 1.5;
		move_bunny_x = 0.0;
		move_bunny_z = -3.5;
		bunny_height = 0.0;
		viewingMatrix = glm::lookAt(glm::vec3(0, 1, 2), glm::vec3(0, 0, 0) + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
		checkpointPosition = -100.0;
		yellowCheckpointIndex = rand() % 3;
		checkpointIsYellow[0] = (0 == yellowCheckpointIndex);
		checkpointIsYellow[1] = (1 == yellowCheckpointIndex);
		checkpointIsYellow[2] = (2 == yellowCheckpointIndex);
		checkpointPositions[0] = glm::vec3(-6.0f, -3.0f, checkpointPosition);
		checkpointPositions[1] = glm::vec3(0.0f, -3.0f, checkpointPosition);
		checkpointPositions[2] = glm::vec3(6.0f, -3.0f, checkpointPosition);
	}
	else if (key == GLFW_KEY_A)
	{
		if(action == GLFW_PRESS)
		{
			aKeyPressed = true;
		}
		else if (action == GLFW_RELEASE)
		{
			aKeyPressed = false;
		}
	}
	else if (key == GLFW_KEY_D)
	{
		if(action == GLFW_PRESS)
		{
			dKeyPressed = true;
		}
		else if (action == GLFW_RELEASE)
		{
			dKeyPressed = false;
		}
	}	
}
void mainLoop(GLFWwindow* window)
{
	while (!glfwWindowShouldClose(window))
	{
		if(!pause){
			display();
			score+= (int)round(bunnySpeed);
			//cout<<score<<"\n";
		}
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
    GLFWwindow* window;
    if (!glfwInit())
    {
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(gWidth, gHeight, "Simple Example", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    char rendererInfo[512] = {0};
    strcpy(rendererInfo, (const char*) glGetString(GL_RENDERER));
    strcat(rendererInfo, " - ");
    strcat(rendererInfo, (const char*) glGetString(GL_VERSION));
    glfwSetWindowTitle(window, rendererInfo);

    init();

    glfwSetKeyCallback(window, keyboard);
    glfwSetWindowSizeCallback(window, reshape);

    reshape(window, gWidth, gHeight); // need to call this once ourselves
    mainLoop(window); // this does not return unless the window is closed

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

