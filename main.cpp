#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <vector>
#include <iostream>
#include <random>
#include <sstream>
#include <fstream>
#include <string>

#define TINYPLY_IMPLEMENTATION
#include <tinyply.h>

float distancez = 10.0f;
const float g = -9.81f;
std::default_random_engine generator;
std::uniform_real_distribution<float> distribution01(0, 1);
std::uniform_real_distribution<float> distributionWorld(-1, 1);
std::uniform_real_distribution<float> distributionSpeedX(0.3f, 1.0f);
std::uniform_real_distribution<float> distributionSpeedY(0.3f, 0.5f);
std::uniform_real_distribution<float> distributionLifeTime(3.0f, 6.0f);
std::uniform_real_distribution<float> distributionMass(0.0001f, 0.0001f);
glm::vec3 spawnPoint = glm::vec3(-0.5f, 0.5f, 0);

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	std::cout << yoffset << std::endl;
	if (yoffset < 0) {
		distancez += 0.1f;
	}
	if (yoffset > 0) {
		distancez -= 0.1f;
	}

}

static void error_callback(int /*error*/, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

/* PARTICULES */
struct Particule {
	glm::vec4 position;
	glm::vec4 color;
	glm::vec4 speed;
};

std::vector<Particule> MakeParticules(const int n)
{


	std::vector<Particule> p;
	p.reserve(n);
	
	for (int i = 0; i < n; i++)
	{
		p.push_back(Particule{
				{
				distributionWorld(generator),
				distributionWorld(generator),
				0.f,
				distributionMass(generator)
				//glm::vec4(spawnPoint.x, spawnPoint.y, spawnPoint.z, distributionMass(generator))
				},
				{
				distribution01(generator),
				distribution01(generator),
				distribution01(generator),
				distributionLifeTime(generator)
				},
				{
					distributionSpeedX(generator),
					distributionSpeedY(generator),
					0.f,
					0.f
				}
				//distributionMass(generator),
				//distributionLifeTime(generator)
			});
	}

	return p;
}

GLuint MakeShader(GLuint t, std::string path)
{
	std::cout << path << std::endl;
	std::ifstream file(path.c_str(), std::ios::in);
	std::ostringstream contents;
	contents << file.rdbuf();
	file.close();

	const auto content = contents.str();
	std::cout << content << std::endl;

	const auto s = glCreateShader(t);

	GLint sizes[] = { (GLint)content.size() };
	const auto data = content.data();

	glShaderSource(s, 1, &data, sizes);
	glCompileShader(s);

	GLint success;
	glGetShaderiv(s, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		GLsizei l;
		glGetShaderInfoLog(s, 512, &l, infoLog);

		std::cout << infoLog << std::endl;
	}

	return s;
}

GLuint AttachAndLink(std::vector<GLuint> shaders)
{
	const auto prg = glCreateProgram();
	for (const auto s : shaders)
	{
		glAttachShader(prg, s);
	}

	glLinkProgram(prg);

	GLint success;
	glGetProgramiv(prg, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		GLsizei l;
		glGetProgramInfoLog(prg, 512, &l, infoLog);

		std::cout << infoLog << std::endl;
	}

	return prg;
}

void APIENTRY opengl_error_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar * message,
	const void* userParam)
{
	std::cout << message << std::endl;
}

int main(void)
{
	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

	window = glfwCreateWindow(1080, 720, "Simple example", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	//glfwSwapInterval(1);
	// NOTE: OpenGL error checks have been omitted for brevity

	if (!gladLoadGL()) {
		std::cerr << "Something went wrong!" << std::endl;
		exit(-1);
	}

	// Callbacks
	glDebugMessageCallback(opengl_error_callback, nullptr);

	const size_t nParticules = 32 * 60000;
	auto particules = MakeParticules(nParticules);

	// Shader
	const auto vertex = MakeShader(GL_VERTEX_SHADER, "shader.vert");
	const auto fragment = MakeShader(GL_FRAGMENT_SHADER, "shader.frag");
	const auto compute = MakeShader(GL_COMPUTE_SHADER, "shader.comp");

	const auto program = AttachAndLink({ vertex, fragment });

	glUseProgram(program);

	const auto compute_program = AttachAndLink({ compute });


	// Buffers
	GLuint vbo, vao;
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, nParticules * sizeof(Particule), particules.data(), GL_STATIC_DRAW);

	// Bindings
	const auto index = glGetAttribLocation(program, "position");

	glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, sizeof(Particule), nullptr);
	glEnableVertexAttribArray(index);

	int uniform_dt = glGetUniformLocation(compute_program, "dt");

	//couleur particules
	const auto particleColor = glGetAttribLocation(program, "colorP");
	glVertexAttribPointer(particleColor, 3, GL_FLOAT, GL_FALSE, sizeof(Particule), (void*)sizeof(glm::vec4));
	glEnableVertexAttribArray(particleColor);

	glPointSize(2.f);

	double lastFrameTime = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		double dt = glfwGetTime() - lastFrameTime;
		lastFrameTime = glfwGetTime();
		std::cout << "fps : " << 1/dt << std::endl;
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glfwSetScrollCallback(window, scroll_callback);

		glProgramUniform1f(compute_program, uniform_dt, dt);

		//-----------------------------VIEW-----------------------------
		/*glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, distancez), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		int uniformView = glGetUniformLocation(program, "view");
		glProgramUniformMatrix4fv(program, uniformView, 1, GL_FALSE, &view[0][0]);*/

		/*//SANS COMPUTE SHADER
		for (int i = 0; i < particules.size(); i++) {
			particules[i].speed.y += particules[i].position.w * g;
			particules[i].position += glm::vec4(particules[i].speed.x * dt, particules[i].speed.y * dt, 0.f, 0.f);

			if (particules[i].position.x > 0.98f)
			{
				particules[i].position.x = 0.98f;
				particules[i].speed = glm::vec4(0.f, particules[i].speed.y, 0.f, 1.f);
			}
			if (particules[i].position.y < -0.98)
			{
				particules[i].position.y = -0.98;
				particules[i].speed = glm::vec4(0, 0, 0, 0);
				particules[i].position.w= 0;
			}

			//on réduit la durée de vie et on test si la particule est morte
			particules[i].color.w -= dt;
			if (particules[i].color.w < 0.0f)
			{
				//tp la particule au spawn
				//Respawn(&particules[i]);
			}
		}*/

		glBufferSubData(GL_ARRAY_BUFFER, 0, nParticules * sizeof(Particule), particules.data());


		//AVEC COMPUTE SHADER
		glUseProgram(compute_program);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo);

		glDispatchCompute(nParticules / 32, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		glUseProgram(program);

		glViewport(0, 0, width, height);

		glClear(GL_COLOR_BUFFER_BIT);
		// glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

		glDrawArrays(GL_POINTS, 0, nParticules);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}