#version 330 core

in vec2 chTexCoord;
out vec4 FragColor;

uniform vec4 uColor;
uniform sampler2D uTexture;
uniform bool uUseTexture;
uniform float uAlpha;

void main() {
    if (uUseTexture) {
        vec4 texColor = texture(uTexture, chTexCoord);
        FragColor = texColor * vec4(1.0, 1.0, 1.0, uAlpha);
    } else {
        FragColor = uColor * vec4(1.0, 1.0, 1.0, uAlpha);
    }
}