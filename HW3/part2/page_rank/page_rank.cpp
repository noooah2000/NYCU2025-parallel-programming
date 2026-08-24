#include "page_rank.h"

#include <cmath>
#include <cstdlib>
#include <omp.h>
#include <algorithm>
#include "../common/graph.h"

// page_rank --
//
// g:           graph to process (see common/graph.h)
// solution:    array of per-vertex vertex scores (length of array is num_nodes(g))
// damping:     page-rank algorithm's damping parameter
// convergence: page-rank algorithm's convergence threshold
//
void page_rank(Graph g, double *solution, double damping, double convergence)
{

    // initialize vertex weights to uniform probability. Double
    // precision scores are used to avoid underflow for large graphs

    int nnodes = num_nodes(g);
    double equal_prob = 1.0 / nnodes;
    for (int i = 0; i < nnodes; ++i)
    {
        solution[i] = equal_prob;
    }

    /*
       For PP students: Implement the page rank algorithm here.  You
       are expected to parallelize the algorithm using openMP.  Your
       solution may need to allocate (and free) temporary arrays.

       Basic page rank pseudocode is provided below to get you started:

       // initialization: see example code above
       score_old[vi] = 1/nnodes;

       while (!converged) {

         // compute score_new[vi] for all nodes vi:
         score_new[vi] = sum over all nodes vj reachable from incoming edges
                            { score_old[vj] / number of edges leaving vj  }
         score_new[vi] = (damping * score_new[vi]) + (1.0-damping) / nnodes;

         score_new[vi] += sum over all nodes v in graph with no outgoing edges
                            { damping * score_old[v] / nnodes }

         // compute how much per-node scores have changed
         // quit once algorithm has converged

         global_diff = sum over all nodes vi { abs(score_new[vi] - score_old[vi]) };
         converged = (global_diff < convergence)
       }
     */
    
    bool converged = false;
    double* score_new;
    double* score_old;
    int* out_degree;
    
    score_new = new double[nnodes];
    score_old = new double[nnodes];
    out_degree = new int[nnodes];

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < nnodes; ++i) {
        score_new[i] = solution[i];
        out_degree[i] = outgoing_size(g, i);
    }

    const double teleport_term = (1.0 - damping) / nnodes;
    const double dangling_scale = damping / nnodes;

    while (!converged) {
        std::swap(score_old, score_new);
        
        double dangling_term = 0.0;
        #pragma omp parallel for schedule(static) reduction(+:dangling_term)
        for (int i = 0; i < nnodes; ++i) {
            if (out_degree[i] == 0){
                dangling_term += score_old[i] * dangling_scale;
            }
        }

        double global_diff = 0.0;
        #pragma omp parallel for schedule(dynamic, 2048) reduction(+:global_diff)
        for (int i = 0; i < nnodes; ++i) {

            double incoming_sum = 0.0;
            const Vertex* start = incoming_begin(g, i);
            const Vertex* end = incoming_end(g, i);
            for (const Vertex* v = start; v != end; ++v) {
                int outdeg = out_degree[*v];
                if (outdeg != 0) incoming_sum += score_old[*v] / outdeg;
            }
            score_new[i] = incoming_sum * damping + teleport_term + dangling_term;
            global_diff += std::abs(score_new[i] - score_old[i]);
        }
        converged = (global_diff < convergence); 
    }
    
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < nnodes; ++i)  solution[i] = score_new[i];
    delete[] score_old;
    delete[] score_new;
    delete[] out_degree;
}