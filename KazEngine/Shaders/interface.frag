#version 450

// layout (set=0, binding=0) uniform sampler2D inTexture;


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
	// ivec4 selection = ivec4(150, 30, 850, 600);
	
	float sel_square = 0;
	if(display_selection > 0) {
	
		ivec4 sel = selection;
		/*if(sel.x < sel.z) {
			int tmp = sel.x;
			sel.x = sel.z;
			sel.z = tmp;
		}*/
	
		/*float sel_square1 = step(sel.x, gl_FragCoord.x) * step(sel.y, gl_FragCoord.y) * step(sel.x, sel.z - gl_FragCoord.x) * step(sel.y, sel.w - gl_FragCoord.y);
		float sel_square2 = step(sel.x + sel_width, gl_FragCoord.x) * step(sel.y + sel_width, gl_FragCoord.y)
						  * step(sel.x, sel.z - sel_width - gl_FragCoord.x) * step(sel.y, sel.w - sel_width - gl_FragCoord.y);
		sel_square = sel_square1 - sel_square2;*/
		
		float sel_square_x1;
		if(sel.x > sel.z) sel_square_x1 = step(gl_FragCoord.x, sel.x) * step(sel.z - gl_FragCoord.x, 0);
		else sel_square_x1 = step(sel.x, gl_FragCoord.x) * step(0, sel.z - gl_FragCoord.x);
		
		float sel_square_y1;
		if(sel.y > sel.w) sel_square_y1 = step(gl_FragCoord.y, sel.y) * step(sel.w - gl_FragCoord.y, 0);
		else sel_square_y1 = step(sel.y, gl_FragCoord.y) * step(0, sel.w - gl_FragCoord.y);
		
		float sel_square_x2;
		if(sel.x > sel.z) sel_square_x2 = step(gl_FragCoord.x, sel.x - sel_width) * step(sel.z + sel_width - gl_FragCoord.x, 0);
		else sel_square_x2 = step(sel.x + sel_width, gl_FragCoord.x) * step(0, sel.z - sel_width - gl_FragCoord.x);
		
		float sel_square_y2;
		if(sel.y > sel.w) sel_square_y2 = step(gl_FragCoord.y, sel.y - sel_width) * step(sel.w + sel_width - gl_FragCoord.y, 0);
		else sel_square_y2 = step(sel.y + sel_width, gl_FragCoord.y) * step(0, sel.w - sel_width - gl_FragCoord.y);
		
		float sel_square_2 = sel_square_x2 * sel_square_y2;
		sel_square = sel_square_x1 * sel_square_y1 - sel_square_x2 * sel_square_y2;
	}

	outColor = vec4(vec3(1.0), sel_square);
}