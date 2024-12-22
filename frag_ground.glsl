#version 330 core

uniform float scale;
uniform float offset;

in vec4 fragWorldPos;

out vec4 fragColor;

void main(void)
{

    int x = int((fragWorldPos.x + offset) * scale) % 2;
	int y = int((fragWorldPos.y + offset) * scale) % 2;
    int z = int((fragWorldPos.z + offset) * scale) % 2;

	bool checker = x!=y;

	if ((checker && z!=1)||(!checker && z==1))
		fragColor = vec4(0.0, 0.0, 0.15,1.0);
	else
		fragColor = vec4(0.4, 0.6, 1.0,1.0);
}
