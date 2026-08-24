#include <mpi.h>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <stdexcept>

static int world, rank;
static int P, Q;
static int n_local, l_local;
static std::vector<int> rb_array, cb_array, rows_len_array, cols_len_array;
static std::vector<MPI_Request> requests;
static std::vector<MPI_Datatype> subarray_types;

static void* aligned_malloc(size_t bytes) {
    void* ptr = nullptr;
    const size_t alignment = 64;
    if (posix_memalign(&ptr, alignment, bytes)) throw std::bad_alloc();
    return ptr;
}

static inline void split_1d(int N, int P, int r, int &begin, int &end) {
    int q = N / P, rem = N % P;
    begin = r * q + std::min(r, rem);
    end   = begin + q + (r < rem ? 1 : 0);
}


void construct_matrices(
    int n, int m, int l, const int *a_mat, const int *b_mat, int **a_mat_ptr, int **b_mat_ptr)
{
    /* TODO: The data is stored in a_mat and b_mat.
     * You need to allocate memory for a_mat_ptr and b_mat_ptr,
     * and copy the data from a_mat and b_mat to a_mat_ptr and b_mat_ptr, respectively.
     * You can use any size and layout you want if they provide better performance.
     * Unambitiously copying the data is also acceptable.
     *
     * The matrix multiplication will be performed on a_mat_ptr and b_mat_ptr.
     */

    MPI_Comm_size(MPI_COMM_WORLD, &world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int dims[2] = {0, 0};
    MPI_Dims_create(world, 2, dims);
    P = dims[0];
    Q = dims[1];

    int row = rank / Q;
    int col = rank % Q;

    int row_begin, row_end, col_begin, col_end;
    split_1d(n, P, row, row_begin, row_end);
    split_1d(l, Q, col, col_begin, col_end);
    n_local = row_end - row_begin;
    l_local = col_end - col_begin;

    *a_mat_ptr = (int*)aligned_malloc((size_t)n_local * m * sizeof(int));
    *b_mat_ptr = (int*)aligned_malloc((size_t)m * l_local * sizeof(int));


    std::vector<int> sendcountsA(world), displsA(world);
    if (rank == 0) 
    {
        for (int r = 0; r < P; ++r) 
        {
            int rb, re; 
            split_1d(n, P, r, rb, re);
            int rows_len = re - rb;

            for (int c = 0; c < Q; ++c) 
            {
                int dest = r * Q + c;
                sendcountsA[dest] = rows_len * m;
                displsA[dest] = rb * m;
            }
        }
    }

    MPI_Scatterv((rank==0 ? a_mat : nullptr),
                 (rank==0 ? sendcountsA.data() : nullptr),
                 (rank==0 ? displsA.data() : nullptr),
                 MPI_INT,
                 *a_mat_ptr,
                 n_local * m,
                 MPI_INT,
                 0,
                 MPI_COMM_WORLD);

    std::vector<int> sendcountsB(world), displsB(world);
    if (rank == 0) 
    {
        for (int c = 0; c < Q; ++c) 
        {
            int cb, ce;
            split_1d(l, Q, c, cb, ce);
            int cols_len = ce - cb;
            for (int r = 0; r < P; ++r) 
            {
                int dest = r * Q + c;
                sendcountsB[dest] = m * cols_len;
                displsB[dest] = cb * m; 
            }
        }
    }

    MPI_Scatterv((rank==0 ? b_mat : nullptr),
                 (rank==0 ? sendcountsB.data() : nullptr),
                 (rank==0 ? displsB.data() : nullptr),
                 MPI_INT,
                 *b_mat_ptr,
                 m * l_local,
                 MPI_INT,
                 0,
                 MPI_COMM_WORLD);

    if (rank == 0) 
    {
        requests.reserve(world - 1);
        subarray_types.reserve(world - 1);
        rb_array.resize(world);
        cb_array.resize(world);
        rows_len_array.resize(world);
        cols_len_array.resize(world);
        for (int i = 1; i < world; ++i) 
        {
            int r = i / Q, c = i % Q;
            int rb, re, cb, ce;
            split_1d(n, P, r, rb, re);
            split_1d(l, Q, c, cb, ce);
            int rows_len = re - rb, cols_len = ce - cb;

            rb_array[i] = rb; 
            cb_array[i] = cb;
            rows_len_array[i] = rows_len;
            cols_len_array[i] = cols_len;
        }
    }

}

void matrix_multiply(
    const int n, const int m, const int l, const int *a_mat, const int *b_mat, int *out_mat)
{
    /* TODO: Perform matrix multiplication on a_mat and b_mat. Which are the matrices you've
     * constructed. The result should be stored in out_mat, which is a continuous memory placing n *
     * l elements of int. You need to make sure rank 0 receives the result.
     */

    if (rank ==0)
    {
        for (int i = 1; i < world; ++i) 
        {
            int rb = rb_array[i], cb = cb_array[i];
            int rows_len = rows_len_array[i];
            int cols_len = cols_len_array[i];

            int sizes[2]    = { n, l };
            int subsizes[2] = { rows_len, cols_len };
            int starts[2]   = { rb, cb };

            MPI_Request req;
            MPI_Datatype type;

            MPI_Type_create_subarray(2, 
                                     sizes, 
                                     subsizes, 
                                     starts,
                                     MPI_ORDER_C, 
                                     MPI_INT, 
                                     &type);
            MPI_Type_commit(&type);
            subarray_types.push_back(type);

            MPI_Irecv(out_mat, 
                      1, 
                      type, 
                      i, 
                      99, 
                      MPI_COMM_WORLD, 
                      &req);  
            requests.push_back(req);
        }
    }


    if (rank == 0) 
    {
        for (int i = 0; i < n_local; ++i) 
        {
            int a_offset = i * m;
            for (int j = 0; j < l_local; ++j) 
            {
                int b_offset = j * m;
                int sum = 0;
                for (int k = 0; k < m; ++k) 
                {
                    sum += a_mat[a_offset + k] * b_mat[b_offset + k];
                }
                out_mat[i * l + j] = sum;
            }
        }

        if (!requests.empty())
            MPI_Waitall(requests.size(), 
                        requests.data(), 
                        MPI_STATUSES_IGNORE);
        for (MPI_Datatype type : subarray_types) MPI_Type_free(&type);
    } 
    else 
    {
        std::vector<int> C_local(n_local * l_local, 0);
        for (int i = 0; i < n_local; ++i) 
        {
            int a_offset = i * m;
            for (int j = 0; j < l_local; ++j) 
            {
                int b_offset = j * m;
                int sum = 0;
                for (int k = 0; k < m; ++k) 
                {
                    sum += a_mat[a_offset + k] * b_mat[b_offset + k];
                }
                C_local[i * l_local + j] = sum;
            }
        }

        MPI_Send(C_local.data(), 
                 C_local.size(),
                 MPI_INT, 
                 0, 
                 99, 
                 MPI_COMM_WORLD);
    }
}

void destruct_matrices(int *a_mat, int *b_mat)
{
    /* TODO */
    if (a_mat) std::free(a_mat);
    if (b_mat) std::free(b_mat);
}
