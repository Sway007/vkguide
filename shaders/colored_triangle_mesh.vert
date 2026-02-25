#version 450
#extension GL_EXT_buffer_reference : require

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUv;

struct Vertex {
    vec4 position;  // xyz = position, w = uv_x
    vec4 normal;    // xyz = normal,   w = uv_y
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(push_constant) uniform constants {
    mat4 render_matrix;
    VertexBuffer vertexBuffer;
} PushConstants;

void main() {
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = PushConstants.render_matrix * vec4(v.position.xyz, 1.0f);
    outColor = v.color.xyz;
    outUv.x = v.position.w;
    outUv.y = v.normal.w;
}
