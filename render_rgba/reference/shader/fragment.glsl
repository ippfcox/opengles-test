#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture_y;
uniform sampler2D texture_u;
uniform sampler2D texture_v;

void main()
{
    vec3 yuv;
    vec3 rgb;
    yuv.x = texture2D(texture_y, TexCoord).r;
    yuv.y = texture2D(texture_u, TexCoord).r - 0.5;
    yuv.z = texture2D(texture_v, TexCoord).r - 0.5;
    rgb = mat3(1, 1, 1,
               0, -0.39465, 2.03211,
               1.13983, -0.58060, 0) * yuv;
    FragColor = vec4(rgb, 1.0);
}