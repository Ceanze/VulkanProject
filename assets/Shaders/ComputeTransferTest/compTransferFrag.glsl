#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 fragPos;
layout(location = 1) in vec3 normal;
layout(location = 0) out vec4 outColor;

void main() {
    double a = 3428095398.0;
    double b = 234235.0;
    for (int i = 0; i < 0; i++) {
        a = sqrt(pow((sin(float(b)) * float(a)) / (float(b) * cos(float(a))), float(b)));
        b = sqrt(pow((sin(float(b)) * float(a)) / (float(a) * cos(float(a))), float(b)));
    }

    vec3 lightDir = normalize(vec3(0.5, -2.0, 0.5));
    float diff = max(dot(normal, -lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.2, 0.6, 0.2 +float( 0.0000001*(a *b / (a/b))));
    outColor = vec4(diffuse, 1.0);//vec4(0.1, abs(fragPos.y) / 100.0, 0.1, 1.0);
}