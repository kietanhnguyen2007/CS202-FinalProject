#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 lightScreenPos;
uniform float screenW;
uniform float screenH;
uniform float radius;
out vec4 finalColor;
void main() {
    vec4 texelColor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    vec2 screenCoord = vec2(gl_FragCoord.x, screenH - gl_FragCoord.y);
    float dist = distance(screenCoord, lightScreenPos);
    float shadow = smoothstep(radius, radius * 0.3f, dist);
    finalColor = vec4(texelColor.rgb * (1.0f - shadow * 0.7f), texelColor.a);
}
