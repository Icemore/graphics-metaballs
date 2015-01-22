#version 330

uniform bool is_wireframe;

uniform struct light_t {
	vec3 pos;
	float ambient;
	float diffuse;
	float specular;
	float shininess;

	vec3 ambient_color;
	vec3 diffuse_color;
} light;

in vec3 norm;
in vec3 position;
out vec3 o_color;

vec3 getBlinnPhong() {
	vec3 pos = position;
	vec3 n = normalize(norm);

	vec3 lightDir = normalize(light.pos - pos);
	vec3 viewDir = normalize(-pos);

	vec3 ambientColor = light.ambient * light.ambient_color;

	float diffuseCoef = light.diffuse * max(dot(lightDir, n), 0);
	vec3 diffuseColor = diffuseCoef * light.diffuse_color;
	diffuseColor = clamp(diffuseColor, 0, 1);
	
	vec3 halfDir = normalize(lightDir + viewDir);
	float specularAngle = max(dot(halfDir, n), 0);
	vec3 specularColor = vec3(light.specular * pow(specularAngle, light.shininess));
	specularColor = clamp(specularColor, 0, 1);

	return ambientColor + diffuseColor + specularColor;
}

void main()
{
    if(is_wireframe) {
        o_color = vec3(1, 0, 0);
    }
    else {
        o_color = getBlinnPhong();
    }
}

