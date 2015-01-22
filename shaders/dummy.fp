#version 330

uniform bool is_wireframe;

in vec4 norm;
out vec3 o_color;

void main()
{
    if(is_wireframe) {
        o_color = vec3(1, 0, 0);
    }
    else {
        float col = norm.y;
        o_color = vec3(col, col, col);
    }
}

