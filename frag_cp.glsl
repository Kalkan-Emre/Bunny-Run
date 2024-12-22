#version 330 core

// All of the following variables could be defined in the OpenGL
// program and passed to this shader as uniform variables. This
// would be necessary if their values could change during runtim.
// However, we will not change them and therefore we define them 
// here for simplicity.

vec3 I = vec3(1, 1, 1);          // point light intensity
vec3 Iamb = vec3(0.7, 0.7, 0.7); // ambient light intensity
vec3 kd = vec3(0.6, 0.0, 0.0);     // diffuse reflectance coefficient
vec3 ka = vec3(0.6, 0.0, 0.0);   // ambient reflectance coefficient
vec3 ks = vec3(0.4, 0.4, 0.4);   // specular reflectance coefficient


uniform vec3 eyePos;
uniform float lightOffset;

uniform int isYellow;

vec3 lightPos = vec3(-8, 30, -3+lightOffset);   // light position in world coordinates

in vec4 fragWorldPos;
in vec3 fragWorldNor;

out vec4 fragColor;

void main(void)
{
	// Compute lighting. We assume lightPos and eyePos are in world
	// coordinates. fragWorldPos and fragWorldNor are the interpolated
	// coordinates by the rasterizer.

	vec3 L = normalize(lightPos - vec3(fragWorldPos));
	vec3 V = normalize(eyePos - vec3(fragWorldPos));
	vec3 H = normalize(L + V);
	vec3 N = normalize(fragWorldNor);

	float NdotL = dot(N, L); // for diffuse component
	float NdotH = dot(N, H); // for specular component

	if(isYellow==1){
		kd = vec3(0.85, 0.6, 0.0);
		ka = vec3(0.85, 0.6, 0.0);
	}

	vec3 diffuseColor = I * kd * max(0, NdotL);
	vec3 specularColor = I * ks * pow(max(0, NdotH), 5);
	vec3 ambientColor = Iamb * ka;

	fragColor = vec4(diffuseColor + specularColor + ambientColor, 1);
}
