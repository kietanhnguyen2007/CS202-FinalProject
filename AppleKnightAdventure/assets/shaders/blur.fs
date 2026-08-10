#version 330 core

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

// Uniform inputs
uniform sampler2D texture0;
uniform vec2 direction;
uniform vec2 resolution;

void main()
{
    float weights[9] = float[9](0.0093, 0.028002, 0.065984, 0.121703, 0.175713, 0.121703, 0.065984, 0.028002, 0.0093);
    
    vec4 sum = vec4(0.0);
    
    for(int i = 0; i < 9; i++)
    {
        vec2 offset = direction * float(i - 4) / resolution;
        sum += texture(texture0, fragTexCoord + offset) * weights[i];
    }
    
    finalColor = sum * fragColor;
}
