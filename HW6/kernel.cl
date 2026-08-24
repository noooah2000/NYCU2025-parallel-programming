__kernel void convolution(__constant float *filter,
                          __global const float *input, 
                          __global float *output, 
                          int filter_width,
                          int height, 
                          int width) 
{
    int j = get_global_id(0);
	int i = get_global_id(1);

	if (j >= width || i >= height) return;

	float sum = 0.0f;
    int halffilter_size = filter_width >> 1;
    

    for (int k = -halffilter_size; k <= halffilter_size; k++) 
    {
        int i_add_k = i + k;
        if (i_add_k < 0 || i_add_k >= height) continue;

    	int row_offset = i_add_k * width;
        int filter_row_offset = (k + halffilter_size) * filter_width;

        for (int l = -halffilter_size; l <= halffilter_size; l++) 
        {
            int j_add_l = j + l;
            if (j_add_l < 0 || j_add_l >= width) continue;

            sum += input[row_offset + j_add_l] * filter[filter_row_offset + l + halffilter_size];         
        }
    }
    output[i * width + j] = sum;
}


__kernel void convolution3x3(__constant float *filter,
                             __global const float *input,
                             __global float *output,
                             int filter_width,
                             int height,
                             int width)
{
    const int j = get_global_id(0);
    const int i = get_global_id(1);

    if (j >= width || i >= height) return;

    const int row1 = (i - 1) * width;
    const int row2 = i * width;
    const int row3 = (i + 1) * width;


    if (j > 0 && j < width - 1 && i > 0 && i < height - 1)
    {
        float sum = 0.0f;

        sum += input[row1 + (j - 1)] * filter[0];
        sum += input[row1 +  j     ] * filter[1];
        sum += input[row1 + (j + 1)] * filter[2];

        sum += input[row2 + (j - 1)] * filter[3];
        sum += input[row2 +  j     ] * filter[4];
        sum += input[row2 + (j + 1)] * filter[5];

        sum += input[row3 + (j - 1)] * filter[6];
        sum += input[row3 +  j     ] * filter[7];
        sum += input[row3 + (j + 1)] * filter[8];

        output[i * width + j] = sum;
        return;
    }

    float sum = 0.0f;
    for (int k = -1; k <= 1; ++k) 
    {
        int i_add_k = i + k;
        if (i_add_k < 0 || i_add_k >= height) continue;

        int in_row = i_add_k * width;
        int f_row  = (k + 1) * 3;

        for (int l = -1; l <= 1; ++l) 
        {
            int j_add_l = j + l;
            if (j_add_l < 0 || j_add_l >= width) continue;

            sum += input[in_row + j_add_l] * filter[f_row + (l + 1)];
        }
    }
    output[i * width + j] = sum;
}
