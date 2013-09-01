/*
 * author: Imran Fanaswala
 */

#ifndef BEAGLE_UTILS_CPP
#define BEAGLE_UTILS_CPP

#include  <stdio.h>
#include "beagle_utils.h"

double* int_to_double(const int* src, int num_elems)
{
    double* dest = (double*)malloc(num_elems*sizeof(double)); if (NULL==dest) Warn_And_Exit("Couldn't allocate memory");
    for(int i=0; i<num_elems;++i)
        dest[i] = (double)src[i];
    return dest;
}

double* short_to_double(const short* src, int num_elems)
{
    double* dest = (double*)malloc(num_elems*sizeof(double)); if (NULL==dest) Warn_And_Exit("Couldn't allocate memory");
    for(int i=0; i<num_elems;++i)
        dest[i] = (double)src[i];
    return dest;
}

double* float_to_double(const phydbl* src, int num_elems)
{
    double* dest = (double*)malloc(num_elems*sizeof(double)); if (NULL==dest) Warn_And_Exit("Couldn't allocate memory");
    for(int i=0; i<num_elems;++i)
        dest[i] = (double)src[i];
    return dest;
}

void print_beagle_flags(long inFlags) {
    if (inFlags & BEAGLE_FLAG_PROCESSOR_CPU)      fprintf(stdout, " PROCESSOR_CPU");
    if (inFlags & BEAGLE_FLAG_PROCESSOR_GPU)      fprintf(stdout, " PROCESSOR_GPU");
    if (inFlags & BEAGLE_FLAG_PROCESSOR_FPGA)     fprintf(stdout, " PROCESSOR_FPGA");
    if (inFlags & BEAGLE_FLAG_PROCESSOR_CELL)     fprintf(stdout, " PROCESSOR_CELL");
    if (inFlags & BEAGLE_FLAG_PRECISION_DOUBLE)   fprintf(stdout, " PRECISION_DOUBLE");
    if (inFlags & BEAGLE_FLAG_PRECISION_SINGLE)   fprintf(stdout, " PRECISION_SINGLE");
    if (inFlags & BEAGLE_FLAG_COMPUTATION_ASYNCH) fprintf(stdout, " COMPUTATION_ASYNCH");
    if (inFlags & BEAGLE_FLAG_COMPUTATION_SYNCH)  fprintf(stdout, " COMPUTATION_SYNCH");
    if (inFlags & BEAGLE_FLAG_EIGEN_REAL)         fprintf(stdout, " EIGEN_REAL");
    if (inFlags & BEAGLE_FLAG_EIGEN_COMPLEX)      fprintf(stdout, " EIGEN_COMPLEX");
    if (inFlags & BEAGLE_FLAG_SCALING_MANUAL)     fprintf(stdout, " SCALING_MANUAL");
    if (inFlags & BEAGLE_FLAG_SCALING_AUTO)       fprintf(stdout, " SCALING_AUTO");
    if (inFlags & BEAGLE_FLAG_SCALING_ALWAYS)     fprintf(stdout, " SCALING_ALWAYS");
    if (inFlags & BEAGLE_FLAG_SCALING_DYNAMIC)    fprintf(stdout, " SCALING_DYNAMIC");
    if (inFlags & BEAGLE_FLAG_SCALERS_RAW)        fprintf(stdout, " SCALERS_RAW");
    if (inFlags & BEAGLE_FLAG_SCALERS_LOG)        fprintf(stdout, " SCALERS_LOG");
    if (inFlags & BEAGLE_FLAG_VECTOR_NONE)        fprintf(stdout, " VECTOR_NONE");
    if (inFlags & BEAGLE_FLAG_VECTOR_SSE)         fprintf(stdout, " VECTOR_SSE");
    if (inFlags & BEAGLE_FLAG_VECTOR_AVX)         fprintf(stdout, " VECTOR_AVX");
    if (inFlags & BEAGLE_FLAG_THREADING_NONE)     fprintf(stdout, " THREADING_NONE");
    if (inFlags & BEAGLE_FLAG_THREADING_OPENMP)   fprintf(stdout, " THREADING_OPENMP");
    if (inFlags & BEAGLE_FLAG_FRAMEWORK_CPU)      fprintf(stdout, " FRAMEWORK_CPU");
    if (inFlags & BEAGLE_FLAG_FRAMEWORK_CUDA)     fprintf(stdout, " FRAMEWORK_CUDA");
    if (inFlags & BEAGLE_FLAG_FRAMEWORK_OPENCL)   fprintf(stdout, " FRAMEWORK_OPENCL");
    fflush(stdout);
}

void print_beagle_resource_list()
{
    BeagleResourceList* rList;
    rList = beagleGetResourceList();
    fprintf(stdout, "\n\tAvailable resources:\n");
    for (int i = 0; i < rList->length; i++) {
        fprintf(stdout, "\t\tResource %i:\n\t\tName : %s\n", i, rList->list[i].name);
        fprintf(stdout, "\t\t\tDesc : %s\n", rList->list[i].description);
        fprintf(stdout, "\t\t\tFlags:");
        print_beagle_flags(rList->list[i].supportFlags);
        fprintf(stdout, "\n");
    }
    fflush(stdout);
}

void print_beagle_instance_details(BeagleInstanceDetails *inst)
{
    int rNumber = inst->resourceNumber;
    fprintf(stdout, "\tUsing resource %i:\n", rNumber);
    fprintf(stdout, "\t\tRsrc Name : %s\n", inst->resourceName);
    fprintf(stdout, "\t\tImpl Name : %s\n", inst->implName);
    fprintf(stdout, "\t\tImpl Desc : %s\n", inst->implDescription);
    fprintf(stdout, "\t\tFlags:");
    fflush(stdout);
    print_beagle_flags(inst->flags);
    fflush(stdout);
}

int create_beagle_instance(t_tree *tree, int quiet)
{
    if(UNINITIALIZED != tree->b_inst){
        fprintf(stdout,"\n\tWARNING: Creating a BEAGLE instance on a tree with an pre-existing BEAGLE instance:%d\n",tree->b_inst);
    }
    if(!quiet){
        print_beagle_resource_list();
    }
    BeagleInstanceDetails inst_d;
    int num_rate_catg = tree->mod->ras->n_catg;
    int num_branches  = 2*tree->n_otu-1; //rooted tree
    //Recall that in PhyML, each edge has a "left" and "right" partial vectors. Therefore,
    //in BEAGLE we have 2*num_branches number of partials.
    //BEAGLE's partials buffer = [ tax1, tax2, ..., taxN, b1Left, b2Left, b3Left,...,bMLeft, b1Rght, b2Rght, b3Rght,...,bMRght] (N taxa, M branches)
    int num_partials  = tree->n_otu + (2*num_branches);
    int num_scales    = 2 + num_rate_catg;
//    DUMP_I(tree->n_otu, num_rate_catg, num_partials, num_branches, tree->mod->ns, tree->n_pattern, tree->mod->whichmodel);
    int beagle_inst = beagleCreateInstance(
                                  tree->n_otu,                /**< Number of tip data elements (input) */
                                  num_partials,               /**< Number of partial buffer (input) */
                                  tree->n_otu,                /**< Number of compact state representation buffers to create (input) */
                                  tree->mod->ns,              /**< Number of states in the continuous-time Markov chain (input) */
                                  tree->n_pattern,            /**< Number of site patterns to be handled by the instance (input) */
                                  1,                          /**< Number of rate matrix eigen-decomposition buffers */
                                  num_branches,               /**< Number of rate matrix buffers (input) */
                                  num_rate_catg,              /**< Number of rate categories (input) */
                                  num_scales,                 /**< Number of scaling buffers */
                                  NULL,                       /**< List of potential resource on which this instance is allowed (input, NULL implies no restriction */
                                  0,			    /**< Length of resourceList list (input) */
                                  BEAGLE_FLAG_FRAMEWORK_CPU | BEAGLE_FLAG_PROCESSOR_CPU | BEAGLE_FLAG_SCALING_MANUAL | BEAGLE_FLAG_EIGEN_COMPLEX | ((sizeof(float)==sizeof(phydbl)) ? BEAGLE_FLAG_PRECISION_SINGLE:BEAGLE_FLAG_PRECISION_DOUBLE),
                                  0,                /**< Bit-flags indicating required implementation characteristics, see BeagleFlags (input) */
                                  &inst_d);
    if (beagle_inst < 0){
        fprintf(stderr, "beagleCreateInstance() failed:%i\n\n",beagle_inst);
        return beagle_inst;
    }

    if(!quiet){
        fprintf(stdout, "\n\tUnique BEAGLE instance id:%i\n", beagle_inst);
        print_beagle_instance_details(&inst_d);
    }

    //Set the tips
    for(int i=0; i<2*tree->n_otu-1; ++i) //taxa+internal nodes
    {
//        Print_Tip_Partials(tree, tree->a_nodes[i]);
        if(tree->a_nodes[i]->tax)
        {
            assert(tree->a_nodes[i]->c_seq->len == tree->n_pattern); // number of compacts sites == number of distinct site patterns
            double* tip = short_to_double(tree->a_nodes[i]->b[0]->p_lk_tip_r, tree->n_pattern*tree->mod->ns); //The tip states are stored on the branch leading to the tip
            //Recall we store tip partials on the branch leading to the tip, rather than the tip itself.
            int ret = beagleSetTipPartials(beagle_inst, tree->a_nodes[i]->b[0]->p_lk_tip_idx, tip);
            if(ret<0){
                fprintf(stderr, "beagleSetTipPartials() on instance %i failed:%i\n\n",beagle_inst,ret);
                Free(tip);
                return ret;
            }
            Free(tip);
        }
    }

//    //Set the equilibrium freqs
//    assert(tree->mod->e_frq->pi->len == tree->mod->ns);
//    int ret = beagleSetStateFrequencies(beagle_inst, 0, tree->mod->e_frq->pi->v);
//    if(ret<0){
//        fprintf(stderr, "beagleSetStateFrequencies() on instance %i failed:%i\n\n",beagle_inst,ret);
//        return ret;
//    }

//    //Set the pattern weights
//    ret = beagleSetPatternWeights(beagle_inst, (const double*)tree->data->wght);
//    if(ret<0){
//        fprintf(stderr, "beagleSetPatternWeights() on instance %i failed:%i\n\n",beagle_inst,ret);
//        return ret;
//    }

    //Computing P-matrix directly in BEAGLE? IOW, doing matrix mult in BEAGLE?
    int whichmodel = tree->mod->whichmodel;
    if((tree->mod->io->datatype == AA || whichmodel==GTR || whichmodel==CUSTOM) && tree->mod->use_m4mod == NO)
    {
        int ret=-1;
        double* eigen_vects     = NULL;
        double* eigen_vects_inv = NULL;
        double* eigen_vals      = NULL;
        double* catg_rates      = NULL;
        //Need to convert to doubles?
        if((sizeof(float)==sizeof(phydbl)))
        {
            eigen_vects     = float_to_double(tree->mod->eigen->r_e_vect, tree->mod->eigen->size*tree->mod->eigen->size);
            eigen_vects_inv = float_to_double(tree->mod->eigen->l_e_vect, tree->mod->eigen->size*tree->mod->eigen->size);
            eigen_vals      = float_to_double(tree->mod->eigen->e_val,tree->mod->eigen->size);
            catg_rates      = float_to_double(tree->mod->ras->gamma_rr->v, num_rate_catg);
        }
        else
        {
            eigen_vects     = tree->mod->eigen->r_e_vect;
            eigen_vects_inv = tree->mod->eigen->l_e_vect;
            eigen_vals      = tree->mod->eigen->e_val;
            catg_rates      = tree->mod->ras->gamma_rr->v;
        }

        //Set the Eigen Decomposition of the Q-matrix
        ret = beagleSetEigenDecomposition(beagle_inst,0,eigen_vects,eigen_vects_inv,eigen_vals);
        if(ret<0){
          fprintf(stderr, "beagleSetEigenDecomposition() on instance %i failed:%i\n\n",beagle_inst,ret);
          Free(eigen_vects);Free(eigen_vects_inv);Free(eigen_vals);Free(catg_rates);
          return ret;
        }

        //Set initial substitution rates (weighted)
        ret = beagleSetCategoryRates(beagle_inst, catg_rates);
        if(ret<0){
            fprintf(stderr, "beagleSetCategoryRates() on instance %i failed:%i\n\n",beagle_inst,ret);
            Free(eigen_vects);Free(eigen_vects_inv);Free(eigen_vals);Free(catg_rates);
            return ret;
        }
        Free(eigen_vects);Free(eigen_vects_inv);Free(eigen_vals);Free(catg_rates);
    }

    tree->mod->b_inst = beagle_inst;
    return beagle_inst;
}

/* Update partial likelihood on edge b on the side of b where
   node d lies.
*/
void update_beagle_partials(t_tree* tree, t_edge* b, t_node* d, bool scale)
{
    /*
               |
               |<- b
               |
               d
              / \
          b1 /   \ b2
            /     \
        n_v1     n_v2
    */

    if(d->tax) //Partial likelihoods are only calculated on internal nodes
      {
        PhyML_Printf("\n== t_node %d is a leaf...",d->num);
        PhyML_Printf("\n== Err in file %s at line %d\n\n",__FILE__,__LINE__);
        Warn_And_Exit("\n");
      }

    //Determine d's "left" and "right" neighbors.
    t_node *n_v1, *n_v2;//d's "left" and "right" neighbor nodes
    phydbl *p_lk,*p_lk_v1,*p_lk_v2;
    phydbl *Pij1,*Pij2;
    int *sum_scale, *sum_scale_v1, *sum_scale_v2;
    int *p_lk_loc;
    int dest_p_idx, child1_p_idx, child2_p_idx, Pij1_idx, Pij2_idx;
    n_v1 = n_v2                 = NULL;
    p_lk = p_lk_v1 = p_lk_v2    = NULL;
    Pij1 = Pij2                 = NULL;
    sum_scale_v1 = sum_scale_v2 = NULL;
    p_lk_loc                    = NULL;
    dest_p_idx = child1_p_idx = child2_p_idx = Pij1_idx = Pij2_idx = UNINITIALIZED;
    Set_All_P_Lk(&n_v1,&n_v2,
                 &p_lk,&sum_scale,&p_lk_loc,
                 &Pij1,&p_lk_v1,&sum_scale_v1,
                 &Pij2,&p_lk_v2,&sum_scale_v2,
                 d,b,tree,
                 &dest_p_idx, &child1_p_idx, &child2_p_idx, &Pij1_idx, &Pij2_idx);


//    fprintf(stdout, "\nUpdating partials on Branch %d (on the side where Node %d lies)\n",b->num,d->num);fflush(stdout);
//    double* p_lk_v1_b = (double*)malloc(tree->mod->ras->n_catg*tree->mod->ns*tree->n_pattern*sizeof(double));if(NULL==p_lk_v1_b) Warn_And_Exit("Couldnt allocate memory");
//    beagleGetPartials(tree->b_inst, child1_p_idx, BEAGLE_OP_NONE, (double*)p_lk_v1_b);
//    double* p_lk_v2_b = (double*)malloc(tree->mod->ras->n_catg*tree->mod->ns*tree->n_pattern*sizeof(double));if(NULL==p_lk_v2_b) Warn_And_Exit("Couldnt allocate memory");
//    beagleGetPartials(tree->b_inst, child2_p_idx, BEAGLE_OP_NONE, (double*)p_lk_v2_b);

//    fprintf(stdout, "Left partials :");fflush(stdout);
//    Dump_Arr_D(p_lk_v1_b,   tree->mod->ras->n_catg*tree->mod->ns*tree->n_pattern);
//    fprintf(stdout, "Right partials:");fflush(stdout);
//    Dump_Arr_D(p_lk_v2_b,   tree->mod->ras->n_catg*tree->mod->ns*tree->n_pattern);
//    Free(p_lk_v1_b);
//    Free(p_lk_v2_b);


    //Create the corresponding BEAGLE operation
    BeagleOperation operations[1] = {{dest_p_idx, BEAGLE_OP_NONE, BEAGLE_OP_NONE, child1_p_idx, Pij1_idx, child2_p_idx, Pij2_idx}};
    //Compute the partials
    int ret = beagleUpdatePartials(tree->b_inst, operations, 1, BEAGLE_OP_NONE);
    if(ret<0){
        fprintf(stderr, "beagleUpdatePartials() on instance %i failed:%i\n\n",tree->b_inst,ret);
        Exit("");
    }
    //Load the computed/updated partial partials
    ret = beagleGetPartials(tree->b_inst, dest_p_idx, BEAGLE_OP_NONE, (double*)p_lk);
    if(ret<0){
        fprintf(stderr, "beagleGetPartials() on instance %i failed:%i\n\n",tree->b_inst,ret);
        Exit("");
    }

    if(scale)
    {
        //Scaling. (btw, the p_lk vector (which is returned by BEAGLE) is indexed based on the memory layout rates * patterns * state)
        int n_patterns = tree->n_pattern;
        phydbl p_lk_lim_inf = (phydbl)P_LK_LIM_INF;
        int dim1 = n_patterns * tree->mod->ns;
        int dim2 = tree->mod->ns;
        int sum_scale_v1_val = 0;
        int sum_scale_v2_val = 0;
        phydbl curr_scaler;
        phydbl smallest_p_lk;
        int curr_scaler_pow, piecewise_scaler_pow;
        curr_scaler = .0;
        curr_scaler_pow = piecewise_scaler_pow = 0;
        for(int site=0;site<n_patterns;++site)
        {
            for(int catg=0;catg<tree->mod->ras->n_catg;++catg)
            {
                smallest_p_lk  =  BIG;

                for(int i=0;i<tree->mod->ns;++i)
                {
                    if(p_lk[catg*dim1+site*dim2+i] < smallest_p_lk)
                        smallest_p_lk = p_lk[catg*dim1+site*dim2+i];
                }
                /* Current scaling values at that site */
                sum_scale_v1_val = (sum_scale_v1)?(sum_scale_v1[catg*n_patterns+site]):(0);
                sum_scale_v2_val = (sum_scale_v2)?(sum_scale_v2[catg*n_patterns+site]):(0);

                sum_scale[catg*n_patterns+site] = sum_scale_v1_val + sum_scale_v2_val;
//                fprintf(stderr,"\n%f,%f,%d",smallest_p_lk,curr_scaler,curr_scaler_pow);

                /* Scaling */
                if(smallest_p_lk < p_lk_lim_inf) //do we need to scale?
                  {
                    curr_scaler_pow = (int)(LOG(p_lk_lim_inf)-LOG(smallest_p_lk))/LOG2;
                    curr_scaler     = (phydbl)((unsigned long long)(1) << curr_scaler_pow);

                    sum_scale[catg*n_patterns+site] += curr_scaler_pow;
//                    fprintf(stderr,"\n%d",sum_scale[catg*n_patterns+site]);

                    do
                      {
                        piecewise_scaler_pow = MIN(curr_scaler_pow,63);
                        curr_scaler = (phydbl)((unsigned long long)(1) << piecewise_scaler_pow);
                        for(int j=0;j<tree->mod->ns;++j)
                          {
                            p_lk[catg*dim1+site*dim2+j] *= curr_scaler;

                            if(p_lk[catg*dim1+site*dim2+j] > BIG)
                              {
                                PhyML_Printf("\n. p_lk_lim_inf = %G smallest_p_lk = %G",p_lk_lim_inf,smallest_p_lk);
                                PhyML_Printf("\n. curr_scaler_pow = %d",curr_scaler_pow);
                                PhyML_Printf("\n. Err in file %s at line %d.",__FILE__,__LINE__);
                                Warn_And_Exit("\n");
                              }
//                            fprintf(stderr,"\n%e",p_lk[catg*dim1+site*dim2+j]);
                          }
                        curr_scaler_pow -= piecewise_scaler_pow;
                      }
                    while(curr_scaler_pow != 0);
                    //Store the scaled partials back into BEAGLE
                    ret = beagleSetPartials(tree->b_inst, dest_p_idx, p_lk);
                    if(ret<0){
                        fprintf(stderr, "beagleSetPartials() on instance %i failed:%i\n\n",tree->b_inst,ret);
                        Exit("");
                    }
                  }
            }
        }
    }

//    fprintf(stdout, "Updated partials:");fflush(stdout);
//    Dump_Arr_D(p_lk, tree->mod->ras->n_catg*tree->mod->ns*tree->n_pattern);

//    int parent_nodes[2] = {d->num, d->num};//The child nodes share the same parent
//    int child_nodes[2] = {n_v1->num, n_v2->num};
//    int p_mat[2] = {b1->num, b2->num};
//    int category_weights = 0;
//    int state_freqs = 0;
//    int cum_scale = BEAGLE_OP_NONE;
//    double* log_lks = (double*)malloc(2*sizeof(double)); if(NULL==log_lks) Warn_And_Exit(__PRETTY_FUNCTION__);
//    ret = beagleCalculateEdgeLogLikelihoods(tree->b_inst, &parent_nodes, &child_nodes, &p_mat, NULL, NULL, &category_weights, &state_freqs, &cum_scale, 2, log_lks, NULL, NULL);
//    if(ret<0){
//        fprintf(stderr, "beagleCalculateEdgeLogLikelihoods() on instance %i failed:%i\n\n",tree->b_inst,ret);
//        Exit("");
//    }
}

int finalize_beagle_instance(t_tree *tree)
{
    if(tree->b_inst >= 0)
    {
        int ret = beagleFinalizeInstance(tree->b_inst);
        if(ret<0) fprintf(stderr, "\nFailed to finalize BEAGLE instance %i: %i\n\n", tree->b_inst, ret);
        return ret;
    }
    return 0;
}


#endif // BEAGLE_UTILS_CPP

