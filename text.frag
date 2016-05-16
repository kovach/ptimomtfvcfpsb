uniform sampler2D texture;
uniform vec3 shading;

void main()
{
    vec2 v = 2.0*(gl_TexCoord[0].xy - vec2(0.5,0.5));
    float r = 1.0-length(v)*length(v);
    gl_FragColor = texture2D(texture, gl_TexCoord[0].xy) * vec4(shading, r);
}
