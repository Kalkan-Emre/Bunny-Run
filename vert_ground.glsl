#version 330 core

uniform mat4 modelingMatrix;
uniform mat4 viewingMatrix;
uniform mat4 projectionMatrix;

layout(location=0) in vec3 inVertex;

out vec4 fragWorldPos;

void main(void)
{
	fragWorldPos = modelingMatrix * vec4(inVertex, 1);
    gl_Position = projectionMatrix * viewingMatrix * modelingMatrix * vec4(inVertex, 1);
}

