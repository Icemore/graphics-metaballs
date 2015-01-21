#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 16) out;

uniform isampler2D triTableTex;
uniform vec4 metaballs[32];
uniform int metaballCnt;

uniform float isoLevel;
uniform vec3 vertDecal[8];
uniform mat4 mvp;

uniform ivec2 edgeToVertex[12] = {
    { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 }, { 0, 4 }, {1, 5},
    { 2, 6 }, {3, 7}
};

vec3 cubePos[8];

int triTableAt(int i, int j) {
    return texelFetch(triTableTex, ivec2(j, i), 0).a;
}

vec3 interpolate(vec3 firstPos, vec3 secondPos, float firstLevel, float secondLevel) {
    float mu = (isoLevel - firstLevel) / (secondLevel - firstLevel);
    return mix(firstPos, secondPos, mu);
}

float calculateLevelForOneMetaball(int id, vec3 pos) {
    vec3 metaballPos = metaballs[id].xyz;
    float potential = metaballs[id].w;

    vec3 r = pos - metaballPos;
    float cur = 1 / dot(r, r);

    return potential * cur;
}

float calculateLevel(vec3 pos) {
    float res = 0;

    for (int i = 0; i < metaballCnt; ++i) {
        res += calculateLevelForOneMetaball(i, pos);
    }

    return res;
}

void makeCube() {
    for (int i = 0; i < 8; ++i) {
        cubePos[i] = gl_in[0].gl_Position.xyz + vertDecal[i];
    }
}


void main() {
    makeCube();

    float levels[8];

    int cubeIndex = 0;
    for (int i = 0; i < 8; ++i) {
        levels[i] = calculateLevel(cubePos[i]);
        
        if (levels[i] < isoLevel) {
            cubeIndex |= (1 << i);
        }
    }
 
    if (cubeIndex == 0 || cubeIndex == 255) {
        return;
    }

    vec3 intersects[12];
    for (int i = 0; i < 12; ++i) {
        int firstIdx = edgeToVertex[i][0];
        int secondIdx = edgeToVertex[i][1];

        intersects[i] = interpolate(cubePos[firstIdx], cubePos[secondIdx],
                                    levels[firstIdx], levels[secondIdx]);
    }
    
    for (int i = 0; triTableAt(cubeIndex, i) != -1; i+=3) {
        for (int j = 0; j < 3; ++j) {
            vec3 pos = intersects[triTableAt(cubeIndex, i + j)];
            gl_Position = mvp * vec4(pos, 1);
            EmitVertex();
        }

        EndPrimitive();
    }
}