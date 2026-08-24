#include "bfs.h"
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <omp.h>

#include "../common/graph.h"

#ifdef VERBOSE
#include "../common/CycleTimer.h"
#include <stdio.h>
#endif // VERBOSE

constexpr int ROOT_NODE_ID = 0;
constexpr int NOT_VISITED_MARKER = -1;

//-----------top down-----------------------------------------------------------
void vertex_set_clear(VertexSet *list)
{
    list->count = 0;
}

void vertex_set_init(VertexSet *list, int count)
{
    list->max_vertices = count;
    list->vertices = new int[list->max_vertices];
    vertex_set_clear(list);
}

void vertex_set_destroy(VertexSet *list)
{
    delete[] list->vertices;
}

// Take one step of "top-down" BFS.  For each vertex on the frontier,
// follow all outgoing edges, and add all neighboring vertices to the
// new_frontier.
void top_down_step(Graph g, VertexSet *frontier, VertexSet *new_frontier, int *distances)
{
    #pragma omp parallel 
    {
        std::vector<int> local_buffer;

        #pragma omp for schedule(dynamic, 512)
        for (int i = 0; i < frontier->count; i++)
        {
            int node = frontier->vertices[i];

            int start_edge = g->outgoing_starts[node];
            int end_edge = (node == g->num_nodes - 1) ? g->num_edges : g->outgoing_starts[node + 1];

            // attempt to add all neighbors to the new frontier
            for (int neighbor = start_edge; neighbor < end_edge; neighbor++)
            {
                Vertex outgoing = g->outgoing_edges[neighbor];

                if (distances[outgoing] != NOT_VISITED_MARKER) continue;

                int dist = distances[node] + 1;
                if (__sync_bool_compare_and_swap(&distances[outgoing], NOT_VISITED_MARKER, dist)) {
                    local_buffer.push_back(outgoing);
                }
            }
        }
        int buffer_size = local_buffer.size();
        int offset = __sync_fetch_and_add(&new_frontier->count, buffer_size);
        std::copy(local_buffer.begin(), local_buffer.end(), new_frontier->vertices + offset);
    }
}

// Implements top-down BFS.
//
// Result of execution is that, for each node in the graph, the
// distance to the root is stored in sol.distances.
void bfs_top_down(Graph graph, solution *sol)
{

    VertexSet list1;
    VertexSet list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    VertexSet *frontier = &list1;
    VertexSet *new_frontier = &list2;

    // initialize all nodes to NOT_VISITED
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    // setup frontier with the root node
    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;

    while (frontier->count != 0)
    {

#ifdef VERBOSE
        double start_time = CycleTimer::current_seconds();
#endif

        vertex_set_clear(new_frontier);

        top_down_step(graph, frontier, new_frontier, sol->distances);

#ifdef VERBOSE
        double end_time = CycleTimer::current_seconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        // swap pointers
        VertexSet *tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;
    }

    // free memory
    vertex_set_destroy(&list1);
    vertex_set_destroy(&list2);
}

//-----------bottom up-----------------------------------------------------------

struct Bitmap {
    std::size_t count;
    std::size_t nwords;
    uint64_t*   bits64;
};

inline void bitmap_clear(Bitmap* bitmap)
{
    for (std::size_t i = 0; i < bitmap->nwords; ++i) bitmap->bits64[i] = 0;
    bitmap->count = 0;
}

void bitmap_init(Bitmap* bitmap, std::size_t count)
{
    bitmap->nwords = (count + 63) / 64;
    bitmap->bits64 = new uint64_t [bitmap->nwords];
    bitmap_clear(bitmap);
}

void bitmap_destroy(Bitmap* bitmap)
{
    delete[] bitmap->bits64;
}

static inline std::size_t _map_idx(int i) { return i >> 6; }
static inline uint64_t _map_offset(int i) { return 1ull << (i & 63u); }

inline bool bitmap_test(Bitmap* bitmap, std::size_t i) 
{
    std::size_t idx = _map_idx(i);
    uint64_t offset = _map_offset(i);
    return (bitmap->bits64[idx] & offset);
}

inline void bitmap_set(Bitmap* bitmap, std::size_t i) {
    std::size_t idx = _map_idx(i);
    uint64_t offset = _map_offset(i);
    bitmap->bits64[idx] |= offset;
    bitmap->count++;

}


void bottom_up_step(Graph g, Bitmap *frontier, Bitmap *new_frontier, int *distances)
{
    std::vector<int> local_buffer[omp_get_max_threads()];

    #pragma omp parallel for schedule(dynamic, 512)
    for (Vertex v = 0; v < g->num_nodes; v++)
    {
        if (distances[v] != NOT_VISITED_MARKER) continue;

        int start_edge = g->incoming_starts[v];
        int end_edge = (v == g->num_nodes - 1) ? g->num_edges : g->incoming_starts[v+1];

        for (int neighbor = start_edge; neighbor < end_edge; neighbor++) 
        {
            Vertex incoming = g->incoming_edges[neighbor];
            if (bitmap_test(frontier, incoming)) {
                distances[v] = distances[incoming] + 1;
                local_buffer[omp_get_thread_num()].push_back(v);
                break;
            }
        }
    }
    for (int i = 0; i < omp_get_max_threads(); i++)
    {
        for (int v : local_buffer[i])
        {
            bitmap_set(new_frontier, v);
        }    
    }
}

void bfs_bottom_up(Graph graph, solution *sol)
{
    // For PP students:
    //
    // You will need to implement the "bottom up" BFS here as
    // described in the handout.
    //
    // As a result of your code's execution, sol.distances should be
    // correctly populated for all nodes in the graph.
    //
    // As was done in the top-down case, you may wish to organize your
    // code by creating subroutine bottom_up_step() that is called in
    // each step of the BFS process.

    Bitmap bitmap1;
    Bitmap bitmap2;
    bitmap_init(&bitmap1, graph->num_nodes);
    bitmap_init(&bitmap2, graph->num_nodes);

    Bitmap *frontier = &bitmap1;
    Bitmap *new_frontier = &bitmap2;

    // initialize all nodes to NOT_VISITED
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    // setup frontier with the root node
    bitmap_set(frontier, ROOT_NODE_ID);
    sol->distances[ROOT_NODE_ID] = 0;

    while (frontier->count != 0)
    {

#ifdef VERBOSE
        double start_time = CycleTimer::current_seconds();
#endif

        bitmap_clear(new_frontier);

        bottom_up_step(graph, frontier, new_frontier, sol->distances);

#ifdef VERBOSE
        double end_time = CycleTimer::current_seconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        // swap pointers
        Bitmap *tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;
    }

    // free memory
    bitmap_destroy(&bitmap1);
    bitmap_destroy(&bitmap2);
}

//-----------hybrid-----------------------------------------------------------

inline void build_outdeg_array(Graph g, std::vector<int>& outdeg) {
    #pragma omp parallel for
    for (int v = 0; v < g->num_nodes; v++) {
        int start_edge = g->outgoing_starts[v];
        int end_edge = (v == g->num_nodes - 1) ? g->num_edges : g->outgoing_starts[v+1];
        outdeg[v] = end_edge - start_edge;
    }
}
inline void vertexset_to_bitmap(VertexSet* vrtexset, Bitmap* bitmap) {
    bitmap_clear(bitmap);
    for (int i = 0; i < vrtexset->count; i++) bitmap_set(bitmap, vrtexset->vertices[i]);
}

inline void bitmap_to_vertexset(const Bitmap* bitmap, VertexSet* vrtexset) {
    vrtexset->count = 0;
    for (int v = 0; v < (int)(bitmap->nwords * 64); v++) {
        if (bitmap_test(const_cast<Bitmap*>(bitmap), v)) {
            vrtexset->vertices[vrtexset->count++] = v;
        }
    }
}

void bfs_hybrid(Graph graph, solution *sol)
{
    // For PP students:
    //
    // You will need to implement the "hybrid" BFS here as
    // described in the handout.

    VertexSet list1;
    VertexSet list2;
    Bitmap bitmap1;
    Bitmap bitmap2;

    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);
    bitmap_init(&bitmap1, graph->num_nodes);
    bitmap_init(&bitmap2, graph->num_nodes);

    VertexSet *frontier_TD = &list1;
    VertexSet *new_frontier_TD = &list2;
    Bitmap *frontier_BU = &bitmap1;
    Bitmap *new_frontier_BU = &bitmap2;

    // initialize all nodes to NOT_VISITED
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    // setup frontier with the root node
    frontier_TD->vertices[frontier_TD->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;

    std::vector<int> outdeg(graph->num_nodes);
    build_outdeg_array(graph, outdeg);

    std::size_t unvisited_edges = 0;
    for (int v = 0; v < graph->num_nodes; v++) unvisited_edges += outdeg[v];
    unvisited_edges -= outdeg[ROOT_NODE_ID];

    const int alpha = 14;
    const int beta = 24;
    int mf, nf;

    bool TD_or_BU = 1; // top down -> 1, bottom up -> 0

    while (true)
    {
        int frontier_count = (TD_or_BU) ? frontier_TD->count : (int)frontier_BU->count;
        if (frontier_count == 0) break;

#ifdef VERBOSE
        double start_time = CycleTimer::current_seconds();
#endif

        if (TD_or_BU) 
        {
            vertex_set_clear(new_frontier_TD);
            top_down_step(graph, frontier_TD, new_frontier_TD, sol->distances);
            
            std::size_t add_visited_edges = 0;
            for (int i = 0; i < new_frontier_TD->count; ++i) 
            {
                add_visited_edges += outdeg[new_frontier_TD->vertices[i]];
            }
            unvisited_edges -= add_visited_edges;

            mf = 0;
            for (int i = 0; i < new_frontier_TD->count; ++i) 
            {
                mf += outdeg[new_frontier_TD->vertices[i]];
            }

            // swap pointers
            std::swap(frontier_TD, new_frontier_TD);
            if (mf * alpha > unvisited_edges){
                TD_or_BU = 0;
                vertexset_to_bitmap(frontier_TD, frontier_BU);
            }
            
        } else
        {
            bitmap_clear(new_frontier_BU);
            bottom_up_step(graph, frontier_BU, new_frontier_BU, sol->distances);

            nf = new_frontier_BU->count;
            // swap pointers
            std::swap(frontier_BU, new_frontier_BU);
            if (nf * beta < graph->num_nodes){
                TD_or_BU = 1;
                bitmap_to_vertexset(frontier_BU, frontier_TD);
            }
        }

#ifdef VERBOSE
        double end_time = CycleTimer::current_seconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

    }

    // free memory
    vertex_set_destroy(&list1);
    vertex_set_destroy(&list2);
    bitmap_destroy(&bitmap1);
    bitmap_destroy(&bitmap2);
}
