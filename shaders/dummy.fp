#version 330

uniform bool is_wireframe;

out vec3 o_color;

void main()
{
    if(is_wireframe) {
        o_color = vec3(1, 0, 0);
    }
    else {
        float col = 1;
        o_color = vec3(col, col, 0);
    }
}

