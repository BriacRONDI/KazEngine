#version 450

layout (location = 0) in vec2 inUV;

layout (set=0, binding=0) uniform Selection
{
	ivec4 selection;
	uint display_selection; 
};

layout (location = 0) out vec4 outColor;

#define sel_width 2

void main()
{
	float sel_square = 0;
	if(display_selection > 0) {
		
		float sel_square_x1;
		if(selection.x > selection.z) sel_square_x1 = step(gl_FragCoord.x, selection.x) * step(selection.z - gl_FragCoord.x, 0);
		else sel_square_x1 = step(selection.x, gl_FragCoord.x) * step(0, selection.z - gl_FragCoord.x);
		
		float sel_square_y1;
		if(selection.y > selection.w) sel_square_y1 = step(gl_FragCoord.y, selection.y) * step(selection.w - gl_FragCoord.y, 0);
		else sel_square_y1 = step(selection.y, gl_FragCoord.y) * step(0, selection.w - gl_FragCoord.y);
		
		float sel_square_x2;
		if(selection.x > selection.z) sel_square_x2 = step(gl_FragCoord.x, selection.x - sel_width) * step(selection.z + sel_width - gl_FragCoord.x, 0);
		else sel_square_x2 = step(selection.x + sel_width, gl_FragCoord.x) * step(0, selection.z - sel_width - gl_FragCoord.x);
		
		float sel_square_y2;
		if(selection.y > selection.w) sel_square_y2 = step(gl_FragCoord.y, selection.y - sel_width) * step(selection.w + sel_width - gl_FragCoord.y, 0);
		else sel_square_y2 = step(selection.y + sel_width, gl_FragCoord.y) * step(0, selection.w - sel_width - gl_FragCoord.y);
		
		float sel_square_2 = sel_square_x2 * sel_square_y2;
		sel_square = sel_square_x1 * sel_square_y1 - sel_square_x2 * sel_square_y2;
	}

	outColor = vec4(vec3(1.0), sel_square);
}