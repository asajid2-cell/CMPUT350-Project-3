// Multiplies everything drawn with it by a colour. The simplest useful fragment shader, and the
// one the shader manager's smoke test registers: if a sprite comes out tinted, the whole path
// (file load, manager lookup, draw call) works.

uniform sampler2D texture;
uniform vec4 tintColor;

void main()
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    gl_FragColor = pixel * tintColor;
}
