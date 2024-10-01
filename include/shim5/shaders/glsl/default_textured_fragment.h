#define DEFAULT_GLSL_TEXTURED_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"\n" \
	"varying vec4 colour;\n" \
	"varying vec2 texcoord;\n" \
	"\n" \
	"void main()\n" \
	"{\n" \
	"	vec4 c = texture2D(tex, texcoord) * colour;\n" \
	"	if (c.a == 0.0) {\n" \
	"		discard;\n" \
	"	}\n" \
	"	gl_FragColor = c;\n" \
	"}\n"
