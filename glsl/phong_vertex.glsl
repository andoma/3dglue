layout (location = 0) in vec3 aPos;

#ifdef PER_VERTEX_NORMAL
layout (location = 1) in vec3 aNor;
#endif

#ifdef PER_VERTEX_COLOR
layout (location = 2) in vec4 aCol;
#endif

#ifdef TEX0
layout (location = 3) in vec2 aUV0;
#endif

uniform mat4 PVM;

uniform mat4 M;

#ifdef PER_VERTEX_NORMAL
out vec3 vo_Normal;
#endif

out vec3 vo_FragPos;
out vec3 vo_Pos;

uniform float per_vertex_color_blend;

out vec3 vo_Color;

#ifdef TEX0
out vec2 vo_UV0;
#endif

void
main()
{
    gl_Position = PVM * vec4(aPos.xyz, 1);
    vo_Pos = aPos.xyz;
    vo_FragPos = (M * vec4(aPos.xyz, 1)).xyz;
#ifdef PER_VERTEX_NORMAL
    vo_Normal = aNor;
#endif

#ifdef PER_VERTEX_COLOR
    vo_Color = mix(vec3(1, 1, 1), aCol.xyz, per_vertex_color_blend);
#else
    vo_Color = vec3(1, 1, 1);
#endif

#ifdef TEX0
    vo_UV0 = aUV0;
#endif
}
