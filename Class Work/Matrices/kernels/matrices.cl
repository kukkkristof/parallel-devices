__kernel void transpose(__global float* matrix, int row_size, int col_size, __global float* transposed_matrix)
{
    int global_x = get_global_id(0);
    int global_y = get_global_id(1);
    if (global_x < row_size && global_y < row_size)
    {
        transposed_matrix[global_x + global_y * row_size] = matrix[global_x * row_size + global_y];
    }

}