layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

#ifdef TEX0
in vec2 vo_UV0[];
out vec2 go_UV0;
#endif

in vec3 vo_Pos[];
in vec3 vo_Color[];

out vec3 go_Color;

in vec3 vo_FragPos[];
out vec3 go_FragPos;

#ifdef PER_VERTEX_NORMAL
in vec3 vo_Normal[];
uniform float face_normal_blend;
#endif

out vec3 go_Normal;

void main()
{
    vec3 a = vo_Pos[1] - vo_Pos[0];
    vec3 b = vo_Pos[2] - vo_Pos[0];
    vec3 N = -normalize(cross(b, a));

    for(int i = 0; i < gl_in.length(); i++)
    {
        gl_Position = gl_in[i].gl_Position;

#ifdef PER_VERTEX_NORMAL
        go_Normal = mix(vo_Normal[i], N, face_normal_blend);
#else
        go_Normal = N;
#endif
        go_FragPos = vo_FragPos[i];
        go_Color = vo_Color[i];
#ifdef TEX0
        go_UV0 = vo_UV0[i];
#endif
        EmitVertex();
    }
    EndPrimitive();
}
