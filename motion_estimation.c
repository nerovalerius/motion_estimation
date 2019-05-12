#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Armin Niedermueller

// video widght and height
# define M 352
# define N 288


// Prototypes
char *motion_estimation(char metric_type[], unsigned char *value_ptr, int frame_number, int width, int height, int macroblock_height,int searchblock_height);
void print_some_values(unsigned char *value_ptr, long frame_size, int row_size, int Y_size, int U_size, int V_size);


// THE MAIN
int main(int argc, char *argv[])
{
	// Variables
	FILE *input_file;
	FILE *output_file;
	long file_size, frame_size;
	unsigned char *video_buffer = 0;
	char *file_name;
	int frame_count, row_size, column_size, Y_size, U_size, V_size;

	// Parameter Checks
	char needle[] = ".yuv";
	fprintf(stdout,"%i Parameters: %s, %s, %s\n", argc-1, argv[1], argv[2], argv[3]);
	if (argc<4){
		fprintf(stderr, "not enough parameters -> use: <filename.plane> <width> <height>\n");
		return -1;
	}
		if (!strstr(argv[1],needle)){
		fprintf(stderr,"first parameter is not a .plane file - exiting!\n");
		return -1;
	}

	// Transform Parameters into proper variables
	file_name = argv[1];
	row_size = atoi(argv[2]);		// = columns
	column_size = atoi(argv[3]);	// = rows


	// Read plane file in binary mode
	input_file = fopen(file_name,"rb");

	// file opened properly?
	if (input_file == NULL){
		fprintf(stderr, "cannot open file: %s\n", file_name);
		return -1;
	}


	// Go to the file end...
	fseek(input_file, 0, SEEK_END);
	// and save the byte offset (number of bytes inside the file)
	file_size = ftell(input_file);
	// Go back to the beginning of the file
	fseek (input_file, 0, SEEK_SET);

	// how many bytes has one single frame ?
	// Y: (X*Y)
	// U: (X*Y)/4
	// V: (X*Y)/4
	// ----------
	// (X*Y) * 1.5
	// Example: (352*288) * 1.5 = 152064
	frame_size = ( row_size * column_size ) * 1.5;

	// Byte offset for Y, U and V part inside a frame
	Y_size = ( row_size * column_size );
	U_size = ( row_size * column_size ) * 0.25;
	V_size = ( row_size * column_size ) * 0.25;


	// how many frames has our sequence? file_size / frame_size
	frame_count = file_size / frame_size;

	// Allocate memory for our Array of Frames
	video_buffer = (unsigned char*) malloc((file_size+1)*sizeof(unsigned char));

	// Read (frame_count) elements, each one with a size of (frame_size) bytes, from the (file) and stores them in the (video_buffer) by ptr
	if (( fread(video_buffer, frame_size, frame_count, input_file) == 0)) {
		fprintf(stderr, "cannot read file\n");
		//return -1;
	}




	// Test Output
	fprintf(stdout,"\nbytes video_buffer: %lu", (file_size+1) * sizeof(char));
	fprintf(stdout,"\nbytes file: %li", file_size);
	fprintf(stdout,"\nframe_count: %i\n\n", frame_count);



	// Write plane file in binary mode
	output_file = fopen("output.plane","wb");

	// Write from video buffer into an output file
	if( fwrite(video_buffer,frame_size,frame_count, output_file) == 0 ){
		fprintf(stderr, "cannot write file\n");
		return -1;
    }

	// Close the in- and output filestream
	fclose(output_file);
	fclose(input_file);


	// Print some values from the video buffer
	print_some_values(video_buffer, frame_size, row_size, Y_size, U_size, V_size);

	// Motion Estimation on one explicit frame
	motion_estimation("SAD", video_buffer, 9, row_size, column_size, 16, 32);
	motion_estimation("MSE", video_buffer, 9, row_size, column_size, 16, 32);



    return 0;
}





// PRINT SOME RANDOM VALUES - FUNCTION
void print_some_values(unsigned char *value_ptr, long frame_size, int row_size, int Y_size, int U_size, int V_size){

	///////////////////////////////////////////////////////// PRINT SOME RANDOM VALUES /////////////////////////////////////////////////////////

	int frame, row, column,i;
	int byte_distance_from_frame_start, byte_distance_from_file_start, byte_distance_from_row_start;
	char plane = 'X';
	unsigned char *buffer_start;
	buffer_start = value_ptr;


	// Print out selected Pixels
	for (i = 0; i < 5; i++){

		// edit the ptr to get some different pixels and planes
		value_ptr+=frame_size;	// go to next frame
		value_ptr+=row_size*2;	// go 2 rows down
		value_ptr+=3;			// go 3 columns right

		// on round 3, go to U plane
		if (i == 3){
			 value_ptr+=Y_size; // go from Y plane to U plane
		}


		// on round 4, go to V plane
		if (i == 4){
			 value_ptr+=U_size; // go from U plane to V plane
		}

		// The byte offset from the beginning of the file
		byte_distance_from_file_start = value_ptr - buffer_start;

		// Find out in which Frame we are in
		frame = 0;
		byte_distance_from_frame_start = byte_distance_from_file_start;
		for(;;){
				if (byte_distance_from_frame_start - frame_size > 0){
					byte_distance_from_frame_start -= frame_size;
					frame++;
				} else {
					break;
				}
		}

		// Find out in which Row we are in
		row = 0;
		// The byte offset from the beginning of the frame
		byte_distance_from_row_start = byte_distance_from_frame_start;

		for(;;){
			if ((byte_distance_from_row_start - row_size) > 0){
				byte_distance_from_row_start -= row_size;

				row++;
			} else {
				break;
			}
		}

		// Find out if the Value is a Y, a U or a V:
		if (byte_distance_from_frame_start <= Y_size){
			plane = 'Y';
		} else if (byte_distance_from_frame_start <= (Y_size +U_size) && byte_distance_from_frame_start > Y_size){
			plane = 'U';
		} else if (byte_distance_from_frame_start <= (U_size + Y_size + V_size) && byte_distance_from_frame_start > (Y_size +U_size)){
			plane = 'V';
		}

		// Find out in which Column we are in
		column = byte_distance_from_row_start;

		// Output of some chosen values
		fprintf(stdout, "Frame %i - Row: %i - Column: %i - Byteoffset: %x - Plane: %c - Value: %i\n",frame, row, column, byte_distance_from_file_start, plane, *value_ptr);
	}

}



// MOTION ESTIMATION - FUNCTION
char *motion_estimation(char metric_type[], unsigned char *value_ptr, int frame_number, int width, int height, int macroblock_height,int searchblock_height){

	// Counters
	int x, y, i, j;
	float temp;

	// DataType Motion Vector
	typedef struct mv {
		char x;
		char y;
	} motion_vector;

	// One Motion Vector for the running loops
	motion_vector temp_motion_vector;
	// Final Motion Vector with best SAD
	motion_vector final_motion_vector;

	// SAD metric
	unsigned int metric = 0;
	float temp_metric = 0;

	// Size of one whole frame
	unsigned int whole_frame = (width * height) * 1.5;

	// Size of only the Y_block of a whole frame
	unsigned int Y_frame = width * height;

	// Set ptr to the first pixel of the Nth frame
	unsigned char* macro_ptr = value_ptr + (frame_number*whole_frame);


	// Left-upper-most pixel of the macro block
	if (width % 2 != 0 && height % 2 != 0){
		macro_ptr += (int)((width/2)) - ((searchblock_height - macroblock_height)/2) + (int)((width*(height/2)) - width * ((searchblock_height - macroblock_height)/2));
	} else if (width % 2 == 0 && height % 2 == 0){							// 352 * 288
		macro_ptr += ((width/2)) - ((searchblock_height - macroblock_height)/2) + (width*((height/2)) - width * ((searchblock_height - macroblock_height)/2));  // 352 / 2 = 176 - range from 0 to 351, so minus 1 = x = 175-8 = 167 for the beginning of the 16x16 block and y =...
	}else if (width % 2 != 0 && height % 2 == 0){
		macro_ptr += (int)((width/2)) - ((searchblock_height - macroblock_height)/2) + (width*(height/2)) - width * ((searchblock_height - macroblock_height)/2);
	} else if (width % 2 == 0 && height % 2 != 0){
		macro_ptr += ((width/2)) - ((searchblock_height - macroblock_height)/2) + (int)((width*(height/2))- width * ((searchblock_height - macroblock_height)/2));
	}


	unsigned char* metric_start_ptr = macro_ptr;		// metric calc starts everytime at the first pixel of the 16x16 macroblock at N Frame


	// Go from Frame N to Frame N-1 - but not when the selected frame is frame 0 - ptr would point to unkown memory address
	if(frame_number <= 0){ return "error: frame number must not be 0"; };

	unsigned char* search_ptr = macro_ptr - whole_frame;
	unsigned char* inner_search_ptr;
	unsigned char* search_start_ptr;	// the ptr at the first pixel of the 16x16 search window in the N-1 frame

	// Distance of the search ptr position to the upper-left-most pixel of the 32x32 window
	temp_motion_vector.y = -(searchblock_height - macroblock_height) / 2;	// -8
	temp_motion_vector.x = -(searchblock_height - macroblock_height) / 2;	// -8

	// Get to the beginning of the macro block - upper-left-most pixel
	// Move ptr on x plane to the upper left most pixel of the 32x32 window
	search_ptr += temp_motion_vector.x;
	// Move ptr on y plane to the upper left most pixel of the 32x32 window
	search_ptr += temp_motion_vector.y * width;

	search_start_ptr = search_ptr;

	// search_start_ptr is right! at beginning of 32x32 block
	metric = UINT_MAX;


	// Go with the smaller 16x16 search window through the whole 32x32 search area
	for(i = 0; i < (searchblock_height - macroblock_height); i++){			// 16 rows down


		temp_motion_vector.x = -(searchblock_height - macroblock_height) / 2;

		for (j = 0; j < (searchblock_height - macroblock_height); j++){	// 16 columns right - each SAD calculaton on one column
			// start each metric calc again in the left upper most pixel in the 16x16 block at N Frame
			macro_ptr = metric_start_ptr;		// The ptr in the N Frame 16x16 block
			inner_search_ptr = search_ptr;

			temp_metric = 0;

			// For each vertical pixel of the 16x16 search block at N Frame
			for (y = 0; y < macroblock_height; y++){

				// For each horizontal pixel of the 16x16 search block at N Frame
				for (x = 0; x < macroblock_height; x++){

					if (strcmp("SAD", metric_type) == 0){
						temp = (*(inner_search_ptr) - *(macro_ptr));

						if(temp < 0){
							temp = -temp;
						}
					} else if (strcmp("MSE", metric_type) == 0){
						temp = ((*inner_search_ptr - *macro_ptr) * (*inner_search_ptr - *macro_ptr)); // squared differences
						temp /= (macroblock_height * (searchblock_height-macroblock_height));  // 1/M*N
					}
						temp_metric += temp;


					// Go with ptr down one step at y plane
					macro_ptr++;				// the ptr in the N Frame 16x16 window
					inner_search_ptr++;			// the ptr in the N-1 Frame 32x32 window
				}


				// Go with ptr down one step at y plane
				macro_ptr += width;				// the ptr in the N Frame 16x16 window
				macro_ptr -= (macroblock_height);
				inner_search_ptr += width;		// the ptr in the N-1 Frame 32x32 window
				inner_search_ptr -= (searchblock_height - macroblock_height);

			}
			// Save the final SAD Metric and Final Motion Vector
			if (temp_metric <= metric){
				metric = temp_metric;
				final_motion_vector.x = temp_motion_vector.x;
				final_motion_vector.y = temp_motion_vector.y;
			}

			temp_motion_vector.x++;
			search_ptr++;										// set ptr to the next column of 32x32 block

		}
		// From e.g -8 to +8
		temp_motion_vector.y++;								// each iteration, go with the motion vector down

		search_ptr += width;									// set ptr from last pixel in row Y to last pixel in row Y+1
		search_ptr -= (searchblock_height - macroblock_height);	// set ptr from last pixel in row Y+1 to first pixel in row Y+1 in order to get to the next row

	}

	// fprintf(stdout,"\nN-1 Frame: x:%i - y:%i - ptr: %x, *ptr: %u", x, y, (search_ptr-value_ptr), *search_ptr);
	fprintf(stdout,"------------------------------------------------------------------");
	fprintf(stdout, "\nBest %s: %i - MV: (%i;%i)\n", metric_type, metric, final_motion_vector.x, final_motion_vector.y);

	// Motion Compensation
	search_ptr = search_start_ptr; // Go to the left-uppermost pixel of the 32x32 search window
	macro_ptr = metric_start_ptr;  // Go to -''- of the 16x16 macroblock

	// Go the the 0;0 Motion Vector
	temp_motion_vector.y = (searchblock_height - macroblock_height) / 2;	// 8
	temp_motion_vector.x = (searchblock_height - macroblock_height) / 2;	// 8

	// Go with Pointer to 0;0 Motion Vector
	search_ptr += temp_motion_vector.x;
	search_ptr += temp_motion_vector.y * width;

	// Go to our final Motion Vector (-4;3)
	search_ptr += final_motion_vector.x;
	search_ptr += final_motion_vector.y * width;


	fprintf(stdout,"\n%s - Motion Compensation:\n", metric_type);

	for (y=0; y < macroblock_height; y++){

			for (x=0; x < macroblock_height; x++){
				fprintf(stdout, "%i  ", (*macro_ptr - *search_ptr));
				search_ptr++;
				macro_ptr++;
			}
			printf("\n");
			search_ptr -= macroblock_height; // Go back to first column
			search_ptr += width;			 // Go to next line
			macro_ptr -= macroblock_height;
			macro_ptr += width;
	}



	return "finished";

}
