
out vec4 FragColor;
in vec3 go_Color;

in vec3 go_FragPos;
in vec3 go_Normal;

uniform vec3 lightPos;
uniform vec3 diffuseColor;

uniform vec3 ambientColor;

uniform vec3 specularColor;
uniform float specularStrength;

uniform float normalColorize;
uniform float alpha;

uniform vec3 viewPos;

#ifdef TEX0
uniform sampler2D tex0;
in vec2 go_UV0;
#endif

void main()
{
  vec3 col = go_Color;

#ifdef TEX0
  col = col * texture(tex0, go_UV0).rgb;
#endif

  vec3 lightDir = normalize(lightPos - go_FragPos);
  vec3 diffuse  = max(dot(go_Normal, lightDir), 0.0) * diffuseColor;

  vec3 viewDir = normalize(viewPos - go_FragPos);
  vec3 reflectDir = reflect(-lightDir, go_Normal);
  vec3 spec = pow(max(dot(viewDir, reflectDir), 0.0), 32) * specularColor;

  vec3 result = (ambientColor + diffuse + spec) * col;

  vec3 ncol = vec3(0.5, 0.5, 0.5) * go_Normal + vec3(0.5, 0.5, 0.5);
  result = mix(result, ncol, normalColorize);

  FragColor = vec4(result, alpha);
}
