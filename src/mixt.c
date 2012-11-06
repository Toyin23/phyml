/*

PhyML:  a program that  computes maximum likelihood phylogenies from
DNA or AA homologous sequences.

Copyright (C) Stephane Guindon. Oct 2003 onward.

All parts of the source except where indicated are distributed under
the GNU public licence. See http://www.opensource.org for details.

*/

#include "mixt.h"

int n_sec1 = 0;

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_All(t_tree *mixt_tree)
{
  t_tree *curr, *next;
  int i;

  curr = mixt_tree;
  next = mixt_tree->next;
  
  do
    {
      MIXT_Chain_String(curr->mod->modelname,next->mod->modelname);      
      MIXT_Chain_String(curr->mod->custom_mod_string,next->mod->custom_mod_string);            
      MIXT_Chain_Scalar_Dbl(curr->mod->kappa,next->mod->kappa);            
      MIXT_Chain_Scalar_Dbl(curr->mod->lambda,next->mod->lambda);
      MIXT_Chain_Scalar_Dbl(curr->mod->br_len_multiplier,next->mod->br_len_multiplier);            
      MIXT_Chain_Scalar_Dbl(curr->mod->mr,next->mod->mr);
      MIXT_Chain_Vector_Dbl(curr->mod->Pij_rr,next->mod->Pij_rr);    
      MIXT_Chain_Vector_Dbl(curr->mod->user_b_freq,next->mod->user_b_freq);
      For(i,2*mixt_tree->n_otu-2) MIXT_Chain_Scalar_Dbl(curr->a_edges[i]->l,next->a_edges[i]->l);
      For(i,2*mixt_tree->n_otu-2) MIXT_Chain_Scalar_Dbl(curr->a_edges[i]->l_old,next->a_edges[i]->l_old);
      MIXT_Chain_Rmat(curr->mod->r_mat,next->mod->r_mat);            
      MIXT_Chain_RAS(curr->mod->ras,next->mod->ras);            
      MIXT_Chain_Efrq(curr->mod->e_frq,next->mod->e_frq);            
      MIXT_Chain_Eigen(curr->mod->eigen,next->mod->eigen);            

      curr = next;
      next = next->next;
    }
  while(next);

  curr = mixt_tree;
  do
    {
      MIXT_Chain_Edges(curr);
      MIXT_Chain_Nodes(curr);
      MIXT_Chain_Sprs(curr);
      MIXT_Chain_Triplets(curr);
      
      curr = curr->next;
    }
  while(curr);


}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_Edges(t_tree *tree)
{
  int i;
  t_edge *b;

  For(i,2*tree->n_otu-3)
    {
      b = tree->a_edges[i];

      if(tree->next)      b->next       = tree->next->a_edges[i];
      if(tree->prev)      b->prev       = tree->prev->a_edges[i];
      if(tree->next_mixt) b->next_mixt  = tree->next_mixt->a_edges[i];
      if(tree->prev_mixt) b->prev_mixt  = tree->prev_mixt->a_edges[i];
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_Nodes(t_tree *tree)
{
  int i;
  t_node *n;

  For(i,2*tree->n_otu-2)
    {
      n = tree->a_nodes[i];

      if(tree->next)      n->next       = tree->next->a_nodes[i];
      if(tree->prev)      n->prev       = tree->prev->a_nodes[i];
      if(tree->next_mixt) n->next_mixt  = tree->next_mixt->a_nodes[i];
      if(tree->prev_mixt) n->prev_mixt  = tree->prev_mixt->a_nodes[i];
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_Sprs(t_tree *tree)
{
  int i;
  
  if(tree->next)      tree->best_spr->next      = tree->next->best_spr;
  if(tree->prev)      tree->best_spr->prev      = tree->prev->best_spr;
  if(tree->next_mixt) tree->best_spr->next_mixt = tree->next_mixt->best_spr;
  if(tree->prev_mixt) tree->best_spr->prev_mixt = tree->prev_mixt->best_spr;

  For(i,2*tree->n_otu-2)
    {
      if(tree->next)      tree->spr_list[i]->next      = tree->next->spr_list[i];
      if(tree->prev)      tree->spr_list[i]->prev      = tree->prev->spr_list[i];
      if(tree->next_mixt) tree->spr_list[i]->next_mixt = tree->next_mixt->spr_list[i];
      if(tree->prev_mixt) tree->spr_list[i]->prev_mixt = tree->prev_mixt->spr_list[i];
    }    
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_Triplets(t_tree *tree)
{
  if(tree->next)      tree->triplet_struct->next      = tree->next->triplet_struct;
  if(tree->prev)      tree->triplet_struct->prev      = tree->prev->triplet_struct;
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_String(t_string *curr, t_string *next)
{
  if(!next)
    {
      return;
    }
  else
    {
      t_string *buff,*last;

      last = NULL;

      /*! Search backward */
      buff = curr;
      while(buff)
        {
          if(buff == next) break;
          buff = buff->prev;
        }
      
      /*! Search forward */
      if(!buff)
        {
          buff = curr;
          while(buff)
            {
              if(buff == next) break;
              buff = buff->next;
            }
        }


      if(!buff) 
        {
          last = curr;
          while(last->next) { last = last->next; }

          last->next = next;
          next->prev = last;
        }
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_Vector_Dbl(vect_dbl *curr, vect_dbl *next)
{
  if(!next)
    {
      return;
    }
  else
    {
      vect_dbl *buff,*last;

      last = NULL;

      buff = curr;
      while(buff)
        {
          if(buff == next) break;
          buff = buff->prev;
        }
      
      /*! Search forward */
      if(!buff)
        {
          buff = curr;
          while(buff)
            {
              if(buff == next) break;
              buff = buff->next;
            }
        }

      if(!buff) 
        {
          last = curr;
          while(last->next) { last = last->next; }

          last->next = next;
          next->prev = last;
        }
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_Scalar_Dbl(scalar_dbl *curr, scalar_dbl *next)
{
  if(!next)
    {
      return;
    }
  else
    {
      scalar_dbl *buff, *last;

      last = NULL;

      buff = curr;
      while(buff)
        {
          if(buff == next) break;
          buff = buff->prev;
        }
      
      /*! Search forward */
      if(!buff)
        {
          buff = curr;
          while(buff)
            {
              if(buff == next) break;
              buff = buff->next;
            }
        }

      if(!buff) 
        {
          last = curr;
          while(last->next) { last = last->next; }

          last->next = next;
          next->prev = last;
        }
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_Rmat(t_rmat *curr, t_rmat *next)
{
  if(!next)
    {
      return;
    }
  else
    {
      t_rmat *buff, *last;

      last = NULL;

      buff = curr;
      while(buff)
        {
          if(buff == next) break;
          buff = buff->prev;
        }
      
      /*! Search forward */
      if(!buff)
        {
          buff = curr;
          while(buff)
            {
              if(buff == next) break;
              buff = buff->next;
            }
        }

      if(!buff)
        {
          last = curr;
          while(last->next) { last = last->next; }

          last->next = next;
          next->prev = last;
        }
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_Efrq(t_efrq *curr, t_efrq *next)
{
  if(!next)
    {
      return;
    }
  else
    {
      t_efrq *buff,*last;

      last = NULL;

      buff = curr;
      while(buff)
        {
          if(buff == next) break;
          buff = buff->prev;
        }
      
      /*! Search forward */
      if(!buff)
        {
          buff = curr;
          while(buff)
            {
              if(buff == next) break;
              buff = buff->next;
            }
        }

      if(!buff) 
        {
          last = curr;
          while(last->next) { last = last->next; }

          last->next = next;
          next->prev = last;
        }
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_Eigen(eigen *curr, eigen *next)
{
  if(!next)
    {
      return;
    }
  else
    {
      eigen *buff,*last;

      last = NULL;

      buff = curr;
      while(buff)
        {
          if(buff == next) break;
          buff = buff->prev;
        }
      
      /*! Search forward */
      if(!buff)
        {
          buff = curr;
          while(buff)
            {
              if(buff == next) break;
              buff = buff->next;
            }
        }

      if(!buff) 
        {
          last = curr;
          while(last->next) { last = last->next; }

          last->next = next;
          next->prev = last;
        }
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Chain_RAS(t_ras *curr, t_ras *next)
{
  if(!next)
    {
      return;
    }
  else
    {
      t_ras *buff,*last;
      
      last = NULL;
      
      buff = curr;
      while(buff)
        {
          if(buff == next) break;
          buff = buff->prev;
        }
      
      /*! Search forward */
      if(!buff)
        {
          buff = curr;
          while(buff)
            {
              if(buff == next) break;
              buff = buff->next;
            }
        }

      if(!buff) 
        {
          last = curr;
          while(last->next) { last = last->next; }

          last->next = next;
          next->prev = last;
        }
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Turn_Branches_OnOff(int onoff, t_tree *mixt_tree)
{
  int i;
  t_tree *tree;

  if(mixt_tree->is_mixt_tree == NO)
    {
      PhyML_Printf("\n== Err in file %s at line %d\n\n",__FILE__,__LINE__);
      Exit("\n");
    }

  tree = mixt_tree;
  
  do
    {
      For(i,2*tree->n_otu-3) tree->a_edges[i]->l->onoff = onoff;
      tree = tree->next;
    }
  while(tree && tree->is_mixt_tree == NO);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

phydbl *MIXT_Get_Lengths_Of_This_Edge(t_edge *mixt_b)
{
  phydbl *lens;
  t_edge *b;
  int n_lens;

  lens = NULL;
  n_lens = 0;

  b = mixt_b;
  do
    {
      
      if(!lens) lens = (phydbl *)mCalloc(1,sizeof(phydbl));
      else      lens = (phydbl *)realloc(lens,(n_lens+1)*sizeof(phydbl));

      lens[n_lens] = b->l->v;
      n_lens++;
      b = b->next;
    }
  while(b);

  return(lens);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Set_Lengths_Of_This_Edge(phydbl *lens, t_edge *mixt_b)
{
  t_edge *b;
  int n_lens;

  n_lens = 0;

  b = mixt_b;
  do
    {
      b->l->v = lens[n_lens];
      n_lens++;
      b = b->next;
    }
  while(b);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Post_Order_Lk(t_node *mixt_a, t_node *mixt_d, t_tree *mixt_tree)
{
  t_tree *tree;
  t_node *a,*d;

  tree = mixt_tree;
  a    = mixt_a;
  d    = mixt_d;

  do
    {
      if(tree->is_mixt_tree)
        {
          tree = tree->next;
          a    = a->next;
          d    = d->next;
        }

      if(tree->mod->ras->invar == NO) Post_Order_Lk(a,d,tree);
      
      tree = tree->next;
      a    = a->next;
      d    = d->next;
    }
  while(tree);
  
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Pre_Order_Lk(t_node *mixt_a, t_node *mixt_d, t_tree *mixt_tree)
{
  t_tree *tree;
  t_node *a,*d;

  tree = mixt_tree;
  a    = mixt_a;
  d    = mixt_d;

  do
    {
      if(tree->is_mixt_tree)
        {
          tree = tree->next;
          a    = a->next;
          d    = d->next;
        }
      
      if(tree->mod->ras->invar == NO) Pre_Order_Lk(a,d,tree);

      tree = tree->next;
      a    = a->next;
      d    = d->next;
    }
  while(tree);
  
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

phydbl MIXT_Lk(t_edge *mixt_b, t_tree *mixt_tree)
{
  t_tree *tree,*cpy_mixt_tree;
  t_edge *b,*cpy_mixt_b;
  phydbl sum_lnL;
  int site, class, br;
  phydbl *sum_scale_left_cat,*sum_scale_rght_cat;
  phydbl sum,tmp;
  int exponent;
  phydbl site_lk_cat,site_lk,log_site_lk,inv_site_lk;
  int num_prec_issue,fact_sum_scale;
  phydbl max_sum_scale,min_sum_scale;
  int ambiguity_check,state;
  int k,l;
  int dim1,dim2;

  tree            = NULL;
  b               = NULL;
  cpy_mixt_tree   = mixt_tree;
  cpy_mixt_b      = mixt_b;

  do /*! Consider each element of the data partition */
    {
      Check_Br_Len_Bounds(mixt_tree);

      if(!cpy_mixt_b) 
        {
          Set_Model_Parameters(mixt_tree->mod);      
        }
      if(!cpy_mixt_b)
        {
          For(br,2*mixt_tree->n_otu-3) Update_PMat_At_Given_Edge(mixt_tree->a_edges[br],mixt_tree);
        }
      else
        {
          Update_PMat_At_Given_Edge(mixt_b,mixt_tree);
        }

      if(!cpy_mixt_b)
        {
          MIXT_Post_Order_Lk(mixt_tree->a_nodes[0],mixt_tree->a_nodes[0]->v[0],mixt_tree);
          if(mixt_tree->both_sides == YES)
            MIXT_Pre_Order_Lk(mixt_tree->a_nodes[0],
                              mixt_tree->a_nodes[0]->v[0],
                              mixt_tree);
        }

      if(!cpy_mixt_b) mixt_b = mixt_tree->a_nodes[0]->b[0];

      sum_scale_left_cat = (phydbl *)mCalloc(mixt_tree->mod->ras->n_catg,sizeof(phydbl));
      sum_scale_rght_cat = (phydbl *)mCalloc(mixt_tree->mod->ras->n_catg,sizeof(phydbl));

      mixt_tree->c_lnL = .0;

      dim1 = mixt_tree->mod->ns;
      dim2 = mixt_tree->mod->ns;

      For(site,mixt_tree->n_pattern)
        {
          b    = mixt_b->next;          
          tree = mixt_tree->next;

          while(tree->mod->ras->invar == YES) 
            {
              tree = tree->next;
              b    = b->next;
              if(!tree || tree->is_mixt_tree == YES)
                {
                  PhyML_Printf("\n== Err in file %s at line %d",__FILE__,__LINE__);
                  Exit("\n");
                }
            }

          ambiguity_check = -1;
          state           = -1;
          
          if((b->rght->tax)                   && 
             (!mixt_tree->mod->s_opt->greedy) &&
             (mixt_tree->data->wght[site] > SMALL))
            {
              ambiguity_check = b->rght->c_seq->is_ambigu[site];              
              if(!ambiguity_check) state = b->rght->c_seq->d_state[site];
            }

          
          do
            {
              if(tree->is_mixt_tree)
                {
                  tree = tree->next;
                  b    = b->next;
                }

              tree->curr_site        = site;
              tree->apply_lk_scaling = NO;

              if(!(tree->mod->ras->invar == YES && mixt_tree->is_mixt_tree == YES) &&
                 (tree->data->wght[tree->curr_site] > SMALL)) 
                {                  
                  site_lk_cat = .0;
                  
                  if((b->rght->tax) && (!tree->mod->s_opt->greedy))
                    {
                      if(!ambiguity_check)
                        {
                          sum = .0;
                          For(l,tree->mod->ns)
                            {
                              sum +=
                                b->Pij_rr[state*dim2+l] *
                                b->p_lk_left[site*dim1+l];
                            }

                          site_lk_cat += sum * tree->mod->e_frq->pi->v[state];
                        }
                      else
                        {
                          For(k,tree->mod->ns)
                            {
                              sum = .0;
                              if(b->p_lk_tip_r[site*dim2+k] > .0)
                                {
                                  For(l,tree->mod->ns)
                                    {
                                      sum +=
                                        b->Pij_rr[k*dim2+l] *
                                        b->p_lk_left[site*dim1+l];
                                    }
                                  
                                  site_lk_cat +=
                                    sum *
                                    tree->mod->e_frq->pi->v[k] *
                                    b->p_lk_tip_r[site*dim2+k];
                                }
                            }
                        }
                    }
                  else
                    {
                      For(k,tree->mod->ns)
                        {
                          sum = .0;
                          if(b->p_lk_rght[site*dim1+k] > .0)
                            {
                              For(l,tree->mod->ns)
                                {
                                  sum +=
                                    b->Pij_rr[k*dim2+l] *
                                    b->p_lk_left[site*dim1+l];
                                }
                              
                              site_lk_cat +=
                                sum *
                                tree->mod->e_frq->pi->v[k] *
                                b->p_lk_rght[site*dim1+k];
                            }
                        }
                    }
                  tree->site_lk_cat[0] = site_lk_cat;
                }

              tree = tree->next;
              b    = b->next;
            }
          while(tree && tree->is_mixt_tree == NO);

          max_sum_scale =  (phydbl)BIG;
          min_sum_scale = -(phydbl)BIG;

          tree  = mixt_tree->next;
          b     = mixt_b->next;
          class = 0;
          do
            {
              if(tree->mod->ras->invar == YES) 
                {
                  tree = tree->next;
                  b    = b->next;
                  if(!(tree && tree->is_mixt_tree == NO)) break;
                }

              sum_scale_left_cat[class] =
                (b->sum_scale_left)?
                (b->sum_scale_left[site]):
                (0.0);                  

              sum_scale_rght_cat[class] =
                (b->sum_scale_rght)?
                (b->sum_scale_rght[site]):
                (0.0);
               
              sum = sum_scale_left_cat[class] + sum_scale_rght_cat[class];
              
              if(sum < .0)
                {
                  PhyML_Printf("\n== sum = %G",sum);
                  PhyML_Printf("\n== Err in file %s at line %d\n\n",__FILE__,__LINE__);
                  Warn_And_Exit("\n");
                }
              
              tmp = sum + ((phydbl)LOGBIG - LOG(tree->site_lk_cat[0]))/(phydbl)LOG2;
              if(tmp < max_sum_scale) max_sum_scale = tmp; /* min of the maxs */
              
              tmp = sum + ((phydbl)LOGSMALL - LOG(tree->site_lk_cat[0]))/(phydbl)LOG2;
              if(tmp > min_sum_scale) min_sum_scale = tmp; /* max of the mins */
              
              class++;

              tree = tree->next;
              b    = b->next;
            }
          while(tree && tree->is_mixt_tree == NO);

          tree = NULL; /*! For debugging purpose */
          
          if(min_sum_scale > max_sum_scale) min_sum_scale = max_sum_scale;
         
          fact_sum_scale = (int)((max_sum_scale + min_sum_scale) / 2);          

          /*! Populate the mixt_tree->site_lk_cat[class] table after
            scaling */

          tree  = mixt_tree->next;
          b     = mixt_b->next;
          class = 0;

          do
            {
              if(tree->mod->ras->invar == YES) 
                {
                  tree = tree->next;
                  b    = b->next;
                  if(!(tree && tree->is_mixt_tree == NO)) break;
                }

              exponent = -(sum_scale_left_cat[class]+sum_scale_rght_cat[class])+fact_sum_scale;
              site_lk_cat = tree->site_lk_cat[0];
              Rate_Correction(exponent,&site_lk_cat,mixt_tree);
              mixt_tree->site_lk_cat[class] = site_lk_cat;
              tree->site_lk_cat[0] = site_lk_cat;
              class++;

              tree = tree->next;
              b    = b->next;
            }
          while(tree && tree->is_mixt_tree == NO);

          site_lk = .0;
          For(class,mixt_tree->mod->ras->n_catg) 
            {
              site_lk += 
                mixt_tree->site_lk_cat[class] * 
                mixt_tree->mod->ras->gamma_r_proba->v[class];
            }


          /* Scaling for invariants */
          if(mixt_tree->mod->ras->invar == YES)
            {
              num_prec_issue = NO;

              tree = mixt_tree->next;
              while(tree->mod->ras->invar == NO)
                {
                  tree = tree->next;
                  if(!tree || tree->is_mixt_tree == YES)
                    {
                      PhyML_Printf("\n== tree: %p",tree);
                      PhyML_Printf("\n== Err in file %s at line %d",__FILE__,__LINE__);
                      Exit("\n");
                    }
                }

              tree->apply_lk_scaling = YES;

              /*! 'tree' will give the correct state frequencies (as opposed to mixt_tree */ 
              inv_site_lk = Invariant_Lk(&fact_sum_scale,site,&num_prec_issue,tree);  

              if(num_prec_issue == YES) // inv_site_lk >> site_lk
                {
                  site_lk = inv_site_lk * mixt_tree->mod->ras->pinvar->v;
                }
              else
                {
                  site_lk = site_lk * (1. - mixt_tree->mod->ras->pinvar->v) + inv_site_lk * mixt_tree->mod->ras->pinvar->v;
                }
            }
          
          log_site_lk = LOG(site_lk) - (phydbl)LOG2 * fact_sum_scale;
                    
          For(class,mixt_tree->mod->ras->n_catg) 
            mixt_tree->log_site_lk_cat[class][site] = 
            LOG(mixt_tree->site_lk_cat[class]) - 
            (phydbl)LOG2 * fact_sum_scale;

          if(isinf(log_site_lk) || isnan(log_site_lk))
            {
              PhyML_Printf("\n== Site = %d",site);
              PhyML_Printf("\n== Invar = %d",mixt_tree->data->invar[site]);
              PhyML_Printf("\n== Mixt = %d",mixt_tree->is_mixt_tree);
              PhyML_Printf("\n== Lk = %G LOG(Lk) = %f < %G",site_lk,log_site_lk,-BIG);
              For(class,mixt_tree->mod->ras->n_catg) PhyML_Printf("\n== rr=%f p=%f",mixt_tree->mod->ras->gamma_rr->v[class],mixt_tree->mod->ras->gamma_r_proba->v[class]);
              PhyML_Printf("\n== Pinv = %G",mixt_tree->mod->ras->pinvar->v);
              PhyML_Printf("\n== Bl mult = %G",mixt_tree->mod->br_len_multiplier->v);
              PhyML_Printf("\n== Err in file %s at line %d",__FILE__,__LINE__);
              Exit("\n");
            }

          mixt_tree->cur_site_lk[site] = log_site_lk;
          
          /* Multiply log likelihood by the number of times this site pattern is found in the data */
          mixt_tree->c_lnL_sorted[site] = mixt_tree->data->wght[site]*log_site_lk;
          
          mixt_tree->c_lnL += mixt_tree->data->wght[site]*log_site_lk;
          /*   tree->sum_min_sum_scale += (int)tree->data->wght[site]*min_sum_scale; */
          
        }
      
      Free(sum_scale_left_cat);
      Free(sum_scale_rght_cat);
      
      mixt_tree = mixt_tree->next_mixt;
      mixt_b    = mixt_b->next_mixt;
    }
  while(mixt_tree);
  
  mixt_tree = cpy_mixt_tree;
  mixt_b    = cpy_mixt_b;

  sum_lnL = .0;
  do
    {
      sum_lnL += mixt_tree->c_lnL;
      mixt_tree = mixt_tree->next_mixt;
    }
  while(mixt_tree);

  mixt_tree = cpy_mixt_tree;
  do
    {
      mixt_tree->c_lnL = sum_lnL;
      mixt_tree = mixt_tree->next_mixt;
    }
  while(mixt_tree);

  mixt_tree = cpy_mixt_tree;
  
  return mixt_tree->c_lnL;
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Update_Eigen(t_mod *mixt_mod)
{
  t_mod *mod;

  mod = mixt_mod;

  do
    {
      if(mod->is_mixt_mod) mod = mod->next;
      Update_Eigen(mod);
      mod = mod->next;
    }
  while(mod);
  
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Update_P_Lk(t_tree *mixt_tree, t_edge *mixt_b, t_node *mixt_d)
{
  t_tree *tree;
  t_edge *b;
  t_node *d;

  tree = mixt_tree;
  b    = mixt_b;
  d    = mixt_d;

  do
    {
      if(tree->is_mixt_tree) 
        {
          tree = tree->next;
          b    = b->next;
          d    = d->next;
        }

      Update_P_Lk(tree,b,d);

      tree = tree->next;
      b    = b->next;
      d    = d->next;

    }
  while(tree);

}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Update_PMat_At_Given_Edge(t_edge *mixt_b, t_tree *mixt_tree)
{
  t_tree *tree;
  t_edge *b;

  tree = mixt_tree;
  b    = mixt_b;

  do
    {
      if(tree->is_mixt_tree) 
        {
          tree = tree->next;
          b    = b->next;
        }
      
      if(tree->mod->ras->invar == NO) Update_PMat_At_Given_Edge(b,tree);

      tree = tree->next;
      b    = b->next;
    }
  while(tree);

}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int *MIXT_Get_Number_Of_Classes_In_All_Mixtures(t_tree *mixt_tree)
{
  int *n_catg;
  t_tree *tree;
  int class;
  
  n_catg = NULL;
  tree = mixt_tree;
  class = 0;
  do
    {
      if(!class) n_catg = (int *)mCalloc(1,sizeof(int));
      else       n_catg = (int *)realloc(n_catg,(class+1)*sizeof(int));

      n_catg[class] = tree->mod->ras->n_catg;
      class++;
      tree = tree->next_mixt;
    }
  while(tree);

  return(n_catg);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

t_tree **MIXT_Record_All_Mixtures(t_tree *mixt_tree)
{
  t_tree **tree_list;
  int n_trees;
  t_tree *tree;

  tree_list = NULL;
  n_trees   = 0;
  tree      = mixt_tree;
  do
    {
      if(!tree_list) tree_list = (t_tree **)mCalloc(1,sizeof(t_tree *));
      else           tree_list = (t_tree **)realloc(tree_list,(n_trees+1)*sizeof(t_tree *));
      
      tree_list[n_trees] = tree;
      n_trees++;
      tree = tree->next;
    }
  while(tree);

  tree_list = (t_tree **)realloc(tree_list,(n_trees+1)*sizeof(t_tree *));
  tree_list[n_trees] = NULL;

  return(tree_list);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Break_All_Mixtures(int c_max, t_tree *mixt_tree)
{
  t_tree *tree;
  int c,i;
  
  if(mixt_tree->is_mixt_tree == NO) return;

  c = 0;
  tree = mixt_tree;
  do
    {
      if(tree->is_mixt_tree) 
        {
          c = 0;
          tree = tree->next;
        }

      if(c == (c_max-1)              && 
         tree->next != NULL          && 
         tree->next->is_mixt_tree == NO) 
        {
          if(tree->mixt_tree->next_mixt == NULL)
            {
              tree->next = NULL;
              For(i,2*tree->n_otu-2) tree->a_edges[i]->next  = NULL;
              For(i,2*tree->n_otu-1) tree->a_nodes[i]->next  = NULL;
              For(i,2*tree->n_otu-2) tree->spr_list[i]->next = NULL;
            }
          else
            {
              tree->next = tree->mixt_tree->next_mixt;
              For(i,2*tree->n_otu-2) tree->a_edges[i]->next  = tree->mixt_tree->next_mixt->a_edges[i];
              For(i,2*tree->n_otu-1) tree->a_nodes[i]->next  = tree->mixt_tree->next_mixt->a_nodes[i];
              For(i,2*tree->n_otu-2) tree->spr_list[i]->next = tree->mixt_tree->next_mixt->spr_list[i];
            }
        }

      tree = tree->next;
      c++;
    }
  while(tree);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Reconnect_All_Mixtures(t_tree **tree_list, t_tree *mixt_tree)
{
  t_tree *tree;
  int n_trees;

  if(mixt_tree->is_mixt_tree == NO) return;

  tree = mixt_tree;
  n_trees = 0;
  do
    {
      tree = tree_list[n_trees];
      if(tree->is_mixt_tree == NO) tree->next = tree_list[n_trees+1];
      n_trees++;
      tree = tree->next;
    }
  while(tree);
  
  MIXT_Chain_All(mixt_tree);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int *MIXT_Record_Has_Invariants(t_tree *mixt_tree)
{
  int *has_invariants;
  t_tree *tree;
  int n_trees;

  has_invariants = NULL;
  tree = mixt_tree;
  n_trees = 0;
  do
    {
      if(!n_trees) has_invariants = (int *)mCalloc(1,sizeof(int));
      else         has_invariants = (int *)realloc(has_invariants,(n_trees+1)*sizeof(int));
      has_invariants[n_trees] = (tree->mod->ras->invar == YES)?1:0;
      n_trees++;
      tree = tree->next;
    }
  while(tree);

  return(has_invariants);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Reset_Has_Invariants(int *has_invariants, t_tree *mixt_tree)
{
  t_tree *tree;
  int n_trees;

  tree = mixt_tree;
  n_trees = 0;
  do
    {
      tree->mod->ras->invar = has_invariants[n_trees];
      n_trees++;
      tree = tree->next;
    }
  while(tree);

}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Check_Invar_Struct_In_Each_Partition_Elem(t_tree *mixt_tree)
{
  if(mixt_tree->is_mixt_tree == NO) return;
  else
    {
      t_tree *tree;
      int n_inv;
      
      n_inv = 0;
      tree = mixt_tree;
      do
        {
          if(tree->is_mixt_tree) 
            {
              tree  = tree->next;
              n_inv = 0;
            }

          if(tree->mod->ras->invar == YES) n_inv++; 

          if(n_inv > 1)
            {
              PhyML_Printf("\n== Found %d classes of the mixture for file '%s' set to",n_inv,tree->mixt_tree->io->in_align_file);
              PhyML_Printf("\n== invariable. Only one such class per mixture is allowed.");
              PhyML_Printf("\n== Err in file %s at line %d\n\n",__FILE__,__LINE__);
              Warn_And_Exit("\n");
            }

          if(tree->mixt_tree->mod->ras->invar == NO && 
             tree->mod->ras->invar == YES)
            {
              PhyML_Printf("\n== Unexpected settings for 'siterates' in a partition element (file '%s')",tree->mixt_tree->io->in_align_file);
              PhyML_Printf("\n== Err in file %s at line %d\n\n",__FILE__,__LINE__);
              Warn_And_Exit("\n");
            }
          
          tree = tree->next;
        }
      while(tree);
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Check_RAS_Struct_In_Each_Partition_Elem(t_tree *mixt_tree)
{
  if(mixt_tree->is_mixt_tree == NO) return;
  else
    {
      t_tree *tree;
      int n_classes;
      
      n_classes = 0;
      tree = mixt_tree;
      do
        {
          if(tree->is_mixt_tree) 
            {
              tree  = tree->next;
              n_classes = 0;
            }

          if(tree && tree->mod->ras->invar == NO) n_classes++;

          if((tree->next && tree->next->is_mixt_tree == YES) || (!tree->next)) /*! current tree is the last element of this mixture */
            {
              if(n_classes != tree->mixt_tree->mod->ras->n_catg)
                {                  
                  PhyML_Printf("\n== %d class%s found in 'partitionelem' for file '%s' while the corresponding",
                               n_classes,
                               (n_classes>1)?"es\0":"\0",
                               tree->mixt_tree->io->in_align_file);
                  PhyML_Printf("\n== 'siterates' element defined %d class%s.",
                               tree->mixt_tree->mod->ras->n_catg,
                               (tree->mixt_tree->mod->ras->n_catg>1)?"es\0":"\0");
                  PhyML_Printf("\n== Err in file %s at line %d\n\n",__FILE__,__LINE__);
                  Warn_And_Exit("\n");
                }
            }

          tree = tree->next;
        }
      while(tree);
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Prune_Subtree(t_node *mixt_a, t_node *mixt_d, t_edge **mixt_target, t_edge **mixt_residual, t_tree *mixt_tree)
{
  t_node *a,*d;
  t_edge *target, *residual;
  t_tree *tree;
  
  MIXT_Turn_Branches_OnOff(OFF,mixt_tree);

  tree     = mixt_tree;
  a        = mixt_a;
  d        = mixt_d;
  target   = *mixt_target;
  residual = *mixt_residual;

  do
    {      
      if(tree->is_mixt_tree) 
        {
          tree     = tree->next;
          a        = a->next;
          d        = d->next;
          target   = target->next;
          residual = residual->next;
        }

      Prune_Subtree(a,d,&target,&residual,tree);
      
      tree     = tree->next;
      a        = a->next;
      d        = d->next;
      target   = target->next;
      residual = residual->next;
    }
  while(tree && tree->is_mixt_tree == NO);

  if(tree) Prune_Subtree(a,d,&target,&residual,tree);

  /*! Turn branches of this mixt_tree to ON after recursive call
    to Prune_Subtree such that, if branches of mixt_tree->next
    point to those of mixt_tree, they are set to OFF when calling
    Prune */
  MIXT_Turn_Branches_OnOff(ON,mixt_tree);

}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Graft_Subtree(t_edge *mixt_target, t_node *mixt_link, t_edge *mixt_residual, t_tree *mixt_tree)
{
  t_edge *target,*residual;
  t_node *link;
  t_tree *tree;

  MIXT_Turn_Branches_OnOff(OFF,mixt_tree);

  tree     = mixt_tree;
  target   = mixt_target;
  residual = mixt_residual;
  link     = mixt_link;
  
  do
    {
      if(tree->is_mixt_tree)
        {
          tree     = tree->next;
          target   = target->next;
          residual = residual->next;
          link     = link->next;
        }

      Graft_Subtree(target,link,residual,tree);

      tree     = tree->next;
      target   = target->next;
      residual = residual->next;
      link     = link->next;
    }
  while(tree && tree->is_mixt_tree == NO);

  if(tree) Graft_Subtree(target,link,residual,tree);

  /*! Turn branches of this mixt_tree to ON after recursive call
    to Graft_Subtree such that, if branches of mixt_tree->next
    point to those of mixt_tree, they are set to OFF when calling
    Graft */
  MIXT_Turn_Branches_OnOff(ON,mixt_tree);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Br_Len_Brent(phydbl prop_min,
                       phydbl prop_max,
                       t_edge *mixt_b, 
                       t_tree *mixt_tree)
{
  t_tree *tree;
  t_edge *b;

  b    = mixt_b;
  tree = mixt_tree;
  
  do
    {
      if(tree->is_mixt_tree)
        {
          tree = tree->next;
          b    = b->next;
        }
      
      Br_Len_Brent(prop_min,prop_max,b,tree);

      b->l->onoff = OFF;      
      tree = tree->next;
      b    = b->next;
    }
  while(tree);

  tree = mixt_tree;
  do
    {      
      MIXT_Turn_Branches_OnOff(ON,tree);
      tree = tree->next_mixt;
    }
  while(tree);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Prepare_Tree_For_Lk(t_tree *mixt_tree)
{
  t_tree *tree;
  
  tree = mixt_tree;
  do
    {
      if(tree->is_mixt_tree) tree = tree->next;
      Prepare_Tree_For_Lk(tree);      
      tree = tree->next;
    }
  while(tree && tree->is_mixt_tree == NO);

  if(tree) Prepare_Tree_For_Lk(tree);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Br_Len_Involving_Invar(t_tree *mixt_tree)
{
  int i;
  scalar_dbl *l;

  For(i,2*mixt_tree->n_otu-3) 
    {
      l = mixt_tree->a_edges[i]->l;
      do
	{
          l->v *= (1.-mixt_tree->mod->ras->pinvar->v);
          l = l->next;
	}
      while(l);
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Br_Len_Not_Involving_Invar(t_tree *mixt_tree)
{
  int i;
  scalar_dbl *l;

  For(i,2*mixt_tree->n_otu-3) 
    {
      l = mixt_tree->a_edges[i]->l;
      do
	{
          l->v /= (1.-mixt_tree->mod->ras->pinvar->v);
          l = l->next;
	}
      while(l);
    }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

phydbl MIXT_Unscale_Br_Len_Multiplier_Tree(t_tree *mixt_tree)
{
  int i;
  scalar_dbl *l;

  For(i,2*mixt_tree->n_otu-3) 
    {
      l = mixt_tree->a_edges[i]->l;
      do
	{
          l->v /= mixt_tree->mod->br_len_multiplier->v;
          l = l->next;
	}
      while(l);
    }
  return(-1);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

phydbl MIXT_Rescale_Br_Len_Multiplier_Tree(t_tree *mixt_tree)
{
  int i;
  scalar_dbl *l;

  For(i,2*mixt_tree->n_otu-3) 
    {
      l = mixt_tree->a_edges[i]->l;
      do
	{
          l->v *= mixt_tree->mod->br_len_multiplier->v;
          l = l->next;
	}
      while(l);
    }
  return(-1);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Set_Alias_Subpatt(int onoff, t_tree *mixt_tree)
{
  t_tree *tree;

  tree = mixt_tree;
  do
    {
      tree->update_alias_subpatt = onoff;
      tree = tree->next;
    }
  while(tree);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Check_Single_Edge_Lens(t_tree *mixt_tree)
{
  t_tree *tree;
  int i;

  tree = mixt_tree->next;
  do
    {
      if(tree->next && tree->next->is_mixt_tree == NO)
        {
          For(i,2*tree->n_otu-3)
            {
              if(tree->a_edges[i]->l != tree->next->a_edges[i]->l)
                {
                  PhyML_Printf("\n== %p %p",tree->a_edges[i]->l,tree->next->a_edges[i]->l);
                  PhyML_Printf("\n== Only one set of edge lengths is allowed ");
                  PhyML_Printf("\n== in a 'partitionelem'. Please fix your XML file.");
                  Exit("\n");
                }
            }
        }
      tree = tree->next;
    }
  while(tree->next && tree->next->is_mixt_tree == NO);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int MIXT_Pars(t_edge *mixt_b, t_tree *mixt_tree)
{
  t_edge *b;
  t_tree *tree;

  b    = mixt_b;
  tree = mixt_tree;

  do
    {      
      b    = b->next_mixt;
      tree = tree->next_mixt;
      
      if(tree) 
        {
          Pars(b,tree);
          mixt_tree->c_pars += tree->c_pars; 
        }
    }
  while(tree);

  tree = mixt_tree;
  do
    {
      tree->c_pars = mixt_tree->c_pars;      
      tree = tree->next_mixt;
    }
  while(tree);
  
  return(mixt_tree->c_pars);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Bootstrap(char *best_tree, xml_node *root)
{
  xml_node *n,*p_elem;
  char *bootstrap;

  n = XML_Search_Node_Name("phyml",NO,root);

  bootstrap = XML_Get_Attribute_Value(n,"bootstrap");

  if(!bootstrap) return;
  else
    {
      int n_boot,i,j,k;
      xml_attr *boot_attr,*seqfile_attr,*out_attr,*boot_out_attr;
      char *orig_align,*boot_out_file_name,*xml_boot_file_name,*buff;
      FILE *boot_fp_in_align,*xml_boot_file_fp;
      option *io;
      align **boot_data,**orig_data;
      int position,elem;
      xml_node *boot_root;
      int pid;
      char *s;

      orig_align = (char *)mCalloc(T_MAX_NAME,sizeof(char));

      xml_boot_file_name = (char *)mCalloc(T_MAX_NAME,sizeof(char));
      strcpy(xml_boot_file_name,"phyml_boot_config.");
      pid = (int)getpid();
      sprintf(xml_boot_file_name+strlen(xml_boot_file_name),"%d",pid);
      strcat(xml_boot_file_name,".xml");

      out_attr = XML_Search_Attribute(root,"outputfile");
      boot_out_file_name = (char *)mCalloc(T_MAX_NAME,sizeof(char));
      strcpy(boot_out_file_name,out_attr->value);
      s = XML_Get_Attribute_Value(root,"run.id");
      if(s)
        {
          strcat(boot_out_file_name,"_"); 
          strcat(boot_out_file_name,s); 
        }



      n_boot = atoi(bootstrap);
      
      io = NULL;
      For(i,n_boot)
        {
          boot_root = XML_Copy_XML_Graph(root);

          /*! Set the number of bootstrap repeats to 0
            in each generated XML file */
          boot_attr = XML_Search_Attribute(boot_root,"bootstrap");
          strcpy(boot_attr->value,"0");

          /*! Set the output file name for each bootstrap analysis */
          boot_out_attr = XML_Search_Attribute(boot_root,"outputfile");          
          buff = (char *)mCalloc(T_MAX_NAME,sizeof(char));
          strcpy(buff,boot_out_attr->value);
          Free(boot_out_attr->value);
          boot_out_attr->value = buff;
          sprintf(boot_out_attr->value+strlen(boot_out_attr->value),"_boot.%d",pid);

          p_elem = boot_root;
          elem   = 0;
          do
            {
              p_elem = XML_Search_Node_Name("partitionelem",YES,p_elem);
              if(!p_elem) break;
              
              io = (option *)Make_Input();
              Set_Defaults_Input(io);

              /*! Get the original sequence file name and the corresponding
                attribute in the XML graph 
              */
              seqfile_attr    = NULL;
              seqfile_attr    = XML_Search_Attribute(p_elem,"filename");

              strcpy(orig_align,seqfile_attr->value);

              /*! Open the original sequence file */
              io->fp_in_align = Openfile(orig_align,0);
              
              /*! Read in the original sequence file */
              orig_data       = Get_Seq(io);
              rewind(io->fp_in_align);


              /*! Read in the original sequence file and put
               it in 'boot_data' structure */
              boot_data       = Get_Seq(io);

              fclose(io->fp_in_align);
              
              /*! Bootstrap resampling: sample from original and put in boot */
              For(j,boot_data[0]->len)
                {
                  position = Rand_Int(0,(int)(boot_data[0]->len-1.0));
                  For(k,io->n_otu)
                    {
                      boot_data[k]->state[j] = orig_data[k]->state[position];
                    }
                }
              
              /*! Modify the sequence file attribute in the original XML 
                graph */
              buff = (char *)mCalloc(T_MAX_NAME,sizeof(char));
              Free(seqfile_attr->value);
              seqfile_attr->value = buff;
              sprintf(seqfile_attr->value,"%s_%d_%d",orig_align,elem,i);

              /*! Open a new sequence file with the modified attribute name */
              boot_fp_in_align = Openfile(seqfile_attr->value,1);
              
              /*! Print the bootstrap data set in it */
              Print_Seq(boot_fp_in_align,boot_data,io->n_otu);
              fclose(boot_fp_in_align);


              Free_Seq(orig_data,io->n_otu);
              Free_Seq(boot_data,io->n_otu);


              Free_Input(io);
              elem++;
            }
          while(p_elem);
          
          /*! Open bootstrap XML file in writing mode */
          xml_boot_file_fp = Openfile(xml_boot_file_name,1);

          /*! Write the bootstrap XML graph */
          XML_Write_XML_Graph(xml_boot_file_fp,boot_root);
          fclose(xml_boot_file_fp);

          /*! Reconstruct the tree */
          PhyML_XML(xml_boot_file_name);

          /*! Remove the bootstrap alignment files */
          p_elem = boot_root;
          do
            {
              p_elem = XML_Search_Node_Name("partitionelem",YES,p_elem);
              if(!p_elem) break;
              seqfile_attr = XML_Search_Attribute(p_elem,"filename");
              /* unlink(seqfile_attr->value); */
            }
          while(p_elem);

          XML_Free_XML_Tree(boot_root);         
        }

      Free(xml_boot_file_name);
      Free(orig_align);
      Free(boot_out_file_name);
    }

}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void MIXT_Set_Pars_Thresh(t_tree *mixt_tree)
{
  t_tree *tree;

  tree = mixt_tree;
  do
    {
      tree->mod->s_opt->pars_thresh = (tree->io->datatype == AA)?(15):(5);
      tree = tree->next_mixt;
    }
  while(tree);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

phydbl MIXT_Get_Mean_Edge_Len(t_edge *mixt_b)
{
  phydbl sum;
  int n;
  scalar_dbl *l;

  l   = mixt_b->l;
  
  sum = .0;
  n   = 0 ;
  do 
    {
      sum += l->v;
      n++;
      l = l->next;
    }
  while(l);
  
  return(sum / (phydbl)n);

}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
