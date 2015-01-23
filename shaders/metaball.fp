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

uniform samplerCube cube_texture;
uniform mat4 view;

in vec3 norm;
in vec3 position;
out vec3 o_color;

#define refract_idx 1.33333

vec3 getReflect(vec3 viewDir, vec3 n) {
	vec3 reflected = reflect(-viewDir, n);
	reflected = vec3(inverse(view) * vec4(reflected, 0));

	return texture(cube_texture, reflected).rgb;
}

vec3 getRefract(vec3 viewDir, vec3 n) {
	const float ratio = 1 / refract_idx;

	vec3 refracted = refract(-viewDir, n, ratio);
	refracted = vec3(inverse(view) * vec4(refracted, 0));

	return texture(cube_texture, refracted).rgb;
}

vec3 getSurfaceColor(vec3 viewDir, vec3 n) {
	vec3 reflected = getReflect(viewDir, n);
	vec3 refracted = getRefract(viewDir, n);

	float ratio = (refract_idx - 1) / (refract_idx + 1);
	float factor = ratio + (1 - ratio) * pow(1 - dot(viewDir, n), 2);
	factor = clamp(factor, 0, 1);

	return mix(refracted, reflected, factor);
}

vec3 getBlinnPhong() {
	vec3 n = normalize(norm);

	vec3 lightDir = normalize(light.pos - position);
	vec3 viewDir = normalize(-position);

	vec3 surfaceColor = getSurfaceColor(viewDir, n);

	vec3 ambientColor = light.ambient * surfaceColor;

	float diffuseCoef = light.diffuse * max(dot(lightDir, n), 0);
	vec3 diffuseColor = diffuseCoef * surfaceColor;
	
	vec3 halfDir = normalize(lightDir + viewDir);
	float specularAngle = max(dot(halfDir, n), 0);
	vec3 specularColor = vec3(light.specular * pow(specularAngle, light.shininess));

	return ambientColor + diffuseColor + specularColor;
}

void main()
{
    if(is_wireframe) {
        o_color = vec3(1, 0, 0);
    }
    else {
        //o_color = getSurfaceColor(normalize(-position), normalize(norm));
		o_color = getBlinnPhong();
    }
}

