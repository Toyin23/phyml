/*

PhyML:  a program that  computes maximum likelihood phylogenies from
DNA or AA homologous sequences.

Copyright (C) Stephane Guindon. Oct 2003 onward.

All parts of the source except where indicated are distributed under
the GNU public licence. See http://www.opensource.org for details.

*/



#include "mcmc.h"

/*********************************************************/

void MCMC(arbre *tree)
{
  int n_moves;


  MCMC_Print_Param(tree->mcmc,tree);

/*   MCMC_Randomize_Lexp(tree); */
/*   MCMC_Randomize_Jumps(tree); */
/*   MCMC_Randomize_Alpha(tree); */
  MCMC_Randomize_Node_Times(tree);
  MCMC_Randomize_Rates(tree);
  MCMC_Randomize_Clock_Rate(tree);
  MCMC_Randomize_Nu(tree);

  RATES_Lk_Rates(tree);
  RATES_Update_Cur_Bl(tree);

  if(tree->rates->lk_approx == NORMAL)
    tree->c_lnL = Dnorm_Multi_Given_InvCov_Det(tree->rates->u_cur_l,
					       tree->rates->u_ml_l,
					       tree->rates->invcov,
					       tree->rates->covdet,
					       2*tree->n_otu-3,YES);
  else Lk(tree);


  n_moves = 11;
  do
    {
      MCMC_Times_Local(tree);
      MCMC_Rates_Local(tree);
      MCMC_Clock_Rate(tree);
      MCMC_Nu(tree);
/*       MCMC_Lexp(tree); */
/*       MCMC_Alpha(tree); */
/*       MCMC_Rates_Global(tree); */
/*       MCMC_Times_Global(tree); */
/*       MCMC_Stick_Rates(tree); */
/*       MCMC_Mixing_Step(tree); */
/*       MCMC_Jumps_Local(tree); */
      RATES_Adjust_Clock_Rate(tree);
    }
  while(tree->mcmc->run < tree->mcmc->n_tot_run);
}

/*********************************************************/

void MCMC_Lexp(arbre *tree)
{
  phydbl cur_lexp,new_lexp;
  phydbl cur_lnL_rates,new_lnL_rates;
/*   phydbl cur_lnL_jps,new_lnL_jps; */
  phydbl u,alpha,prior_mean_lexp,ratio;
  
  if((tree->rates->model != COMPOUND_NOCOR) &&
     (tree->rates->model != COMPOUND_COR)) return;

  new_lnL_rates   = UNLIKELY;
  cur_lexp        = -1.0;
  new_lexp        = -1.0;
  prior_mean_lexp = 0.03;
  ratio           = -1.0;

  cur_lnL_rates = tree->rates->c_lnL;
/*   cur_lnL_jps   = RATES_Lk_Jumps(tree); */

  cur_lexp = tree->rates->lexp;
  
  if(cur_lexp > 2.0) printf("\n. cur_lexp = %f iter=%d",cur_lexp,tree->mcmc->run);

  u = Uni();
  new_lexp = cur_lexp * exp(H_MCMC_LEXP*(u-0.5));

  if((new_lexp  > 1.E-5) && (new_lexp  < 2.0))
    {
      tree->rates->lexp = new_lexp;
      
      new_lnL_rates = RATES_Lk_Rates(tree);
/*       new_lnL_jps   = RATES_Lk_Jumps(tree); */

/*       ratio = (new_lnL_rates + new_lnL_jps) - (cur_lnL_rates + cur_lnL_jps) + log(new_lexp/cur_lexp); */
      ratio = (new_lnL_rates) - (cur_lnL_rates) + log(new_lexp/cur_lexp);

      ratio = exp(ratio);

/*       ratio =  */
/* 	exp(new_lnL-cur_lnL)* */
/* 	(new_lexp/cur_lexp) * */
/* 	exp((1./prior_mean_lexp)*(cur_lexp-new_lexp)); */
      
      alpha = MIN(1.,ratio);
      
      u = Uni();
      if(u > alpha) /* Reject */
	{
	  tree->rates->lexp = cur_lexp;
	  RATES_Lk_Rates(tree);
	}
      else
	{
	  tree->mcmc->acc_lexp++;
	}
    }

  tree->mcmc->run++;
  MCMC_Print_Param(tree->mcmc,tree);
}

/*********************************************************/

void MCMC_Nu(arbre *tree)
{
  phydbl cur_nu,new_nu,cur_lnL,new_lnL;
  phydbl u,alpha,prior_mean_nu,ratio;

  if(
     (tree->rates->model == COMPOUND_COR)   || 
     (tree->rates->model == COMPOUND_NOCOR) ||
     (tree->rates->model == GAMMA) ||
     (tree->rates->model == EXPONENTIAL)
     ) return;

  cur_lnL       = UNLIKELY;
  new_lnL       = UNLIKELY;
  cur_nu        = -1.0;
  new_nu        = -1.0;
  prior_mean_nu =  1.0;
  ratio         = -1.0;

  cur_lnL = tree->rates->c_lnL;
  cur_nu  = tree->rates->nu;
  
  u = Uni();
  new_nu = cur_nu * exp(H_MCMC_NU*(u-0.5));

  if(new_nu < tree->rates->min_nu)
    {
      new_nu = tree->rates->max_nu - fmod(new_nu,tree->rates->max_nu-tree->rates->min_nu);
    }
  if(new_nu > tree->rates->max_nu)
    {
      new_nu = tree->rates->min_nu + fmod(new_nu,tree->rates->max_nu-tree->rates->min_nu);
    }
  if(new_nu > tree->rates->max_nu || new_nu < tree->rates->min_nu)
    {      
      PhyML_Printf("\n Problem with setting autocorrelation parameter.\n");
      PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
      Warn_And_Exit("");
    }

  tree->rates->nu = new_nu;  
  new_lnL = RATES_Lk_Rates(tree);  
  ratio = (new_lnL-cur_lnL) + log(new_nu) - log(cur_nu);  
  ratio = exp(ratio);
  alpha = MIN(1.,ratio);
  
  u = Uni();
  if(u > alpha) /* Reject */
    {
      tree->rates->nu = cur_nu;
      RATES_Lk_Rates(tree);
    }
  else
    {
      tree->mcmc->acc_nu++;
    }


  tree->mcmc->run++;
  MCMC_Print_Param(tree->mcmc,tree);

}

/*********************************************************/

void MCMC_Clock_Rate(arbre *tree)
{
  phydbl cur_cr,new_cr,cur_lnL,new_lnL;
  phydbl u,alpha,prior_mean_cr,ratio;
  
  cur_lnL       = UNLIKELY;
  new_lnL       = UNLIKELY;
  cur_cr        = -1.0;
  new_cr        = -1.0;
  prior_mean_cr =  1.0;
  ratio         = -1.0;

  cur_lnL = tree->c_lnL;
  cur_cr  = tree->rates->clock_r;
  
  u = Uni();
  new_cr = cur_cr * exp(H_MCMC_NU*(u-0.5));

  if(new_cr < tree->rates->min_clock)
    {
      new_cr = tree->rates->max_clock - fmod(new_cr,tree->rates->max_clock-tree->rates->min_clock);
    }
  if(new_cr > tree->rates->max_clock)
    {
      new_cr = tree->rates->min_clock + fmod(new_cr,tree->rates->max_clock-tree->rates->min_clock);
    }
  if(new_cr > tree->rates->max_clock || new_cr < tree->rates->min_clock)
    {
      PhyML_Printf("\n Problem with setting new clock rate.\n");
      PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
      Warn_And_Exit("");
    }

  tree->rates->clock_r = new_cr;
  RATES_Update_Cur_Bl(tree);


  if(tree->rates->lk_approx == NORMAL)
    new_lnL = Dnorm_Multi_Given_InvCov_Det(tree->rates->u_cur_l,tree->rates->u_ml_l,tree->rates->invcov,tree->rates->covdet,2*tree->n_otu-3,YES);
  else 
    new_lnL = Return_Lk(tree);
  
  tree->c_lnL = new_lnL;

  ratio = (new_lnL + log(new_cr)) - (cur_lnL + log(cur_cr));
  ratio = exp(ratio);
  
  alpha = MIN(1.,ratio);
  u = Uni();
  if(u > alpha) /* Reject */
    {
      tree->rates->clock_r = cur_cr;

      RATES_Update_Cur_Bl(tree);
      
      if(tree->rates->lk_approx == NORMAL)
	new_lnL = Dnorm_Multi_Given_InvCov_Det(tree->rates->u_cur_l,tree->rates->u_ml_l,tree->rates->invcov,tree->rates->covdet,2*tree->n_otu-3,YES);
      else 
	new_lnL = Return_Lk(tree);

      if(fabs(new_lnL - cur_lnL) > 1.E-3)
	{
	  PhyML_Printf("\n. run=%d",tree->mcmc->run);
	  PhyML_Printf("\n. clock rate=%f",cur_cr);
	  PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
	  Warn_And_Exit("");
	}

      tree->c_lnL = new_lnL;
    }

  tree->mcmc->run++;
  MCMC_Print_Param(tree->mcmc,tree);
}

/*********************************************************/

void MCMC_Alpha(arbre *tree)
{
  phydbl cur_lnL,new_lnL,cur_alpha,new_alpha;
  phydbl u,alpha,ratio;
  
  if((tree->rates->model != COMPOUND_NOCOR) &&
     (tree->rates->model != COMPOUND_COR)   &&
     (tree->rates->model != GAMMA)) return;

  cur_lnL = UNLIKELY;
  new_lnL = UNLIKELY;
  ratio = -1.0;

  cur_lnL    = tree->rates->c_lnL;
  cur_alpha  = tree->rates->alpha;
  
  u =  Uni();
  new_alpha = cur_alpha + (u * 2.*cur_alpha/10. - cur_alpha/10.);

  if(new_alpha > 0.1 && new_alpha < 10.0)
    {
      tree->rates->alpha = new_alpha;
      new_lnL = RATES_Lk_Rates(tree);      
      ratio = exp(new_lnL-cur_lnL);
      alpha = MIN(1.,ratio);
      u = Uni();
 
      if(u > alpha) /* Reject */
	{
	  tree->rates->alpha = cur_alpha;
	  RATES_Lk_Rates(tree);
	}
    }
  
  tree->mcmc->run++;
  MCMC_Print_Param(tree->mcmc,tree);

}

/*********************************************************/

void MCMC_Times_Local(arbre *tree)
{
  int local;
  int node_num;

  local = 1;
  
  node_num = Rand_Int(0,2*tree->n_otu-2);

  if(tree->noeud[node_num] == tree->n_root)
    MCMC_Time_Root(tree);
  else
    MCMC_Times_Pre(tree->noeud[node_num]->anc,tree->noeud[node_num],local,tree);

/*   MCMC_Times_Pre(tree->n_root,tree->n_root->v[0],local,tree); */
/*   MCMC_Times_Pre(tree->n_root,tree->n_root->v[1],local,tree); */
}

/*********************************************************/

void MCMC_Rates_Local(arbre *tree)
{
  int local;
  int node_num;

  local = 1;

  node_num = Rand_Int(0,2*tree->n_otu-3);
  MCMC_Rates_Pre(tree->noeud[node_num]->anc,tree->noeud[node_num],local,tree);

/*   MCMC_Rates_Pre(tree->n_root,tree->n_root->v[0],local,tree); */
/*   MCMC_Rates_Pre(tree->n_root,tree->n_root->v[1],local,tree); */

}

/*********************************************************/

void MCMC_Stick_Rates(arbre *tree)
{
  tree->both_sides = 1;
  Lk(tree);

  MCMC_Stick_Rates_Pre(tree->n_root,tree->n_root->v[0],tree);
  MCMC_Stick_Rates_Pre(tree->n_root,tree->n_root->v[1],tree);
}

/*********************************************************/

void MCMC_Rates_Pre(node *a, node *d, int local, arbre *tree)
{
  phydbl u;
  phydbl new_lnL_data, cur_lnL_data, new_lnL_rate, cur_lnL_rate;
  phydbl ratio, alpha;
  phydbl new_mu, cur_mu;
  int i;
  edge *b;

  b = NULL;
  
  cur_mu       = tree->rates->nd_r[d->num];
  cur_lnL_data = tree->c_lnL;
  cur_lnL_rate = tree->rates->c_lnL;

  if(a == tree->n_root) b = tree->e_root;
  else For(i,3) if(d->v[i] == a) {b = d->b[i]; break;}
  
  u = Uni();
  new_mu = cur_mu * exp(H_MCMC_RATES*(u-0.5));
  
  if(new_mu < tree->rates->min_rate)
    {
      new_mu = tree->rates->max_rate - fmod(new_mu,tree->rates->max_rate-tree->rates->min_rate);
    }
  if(new_mu > tree->rates->max_rate)
    {
      new_mu = tree->rates->min_rate + fmod(new_mu,tree->rates->max_rate-tree->rates->min_rate);
    }
  if(new_mu < tree->rates->min_rate || new_mu > tree->rates->max_rate)
    {
      PhyML_Printf("\n Problem with setting new rate.\n");
      PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
      Warn_And_Exit("");
    }       

  if(local)
    {
      tree->rates->nd_r[d->num] = new_mu;
      RATES_Update_Cur_Bl(tree);

      if(tree->rates->lk_approx == NORMAL)
	new_lnL_data = Dnorm_Multi_Given_InvCov_Det(tree->rates->u_cur_l,tree->rates->u_ml_l,tree->rates->invcov,tree->rates->covdet,2*tree->n_otu-3,YES);
      else
	new_lnL_data = Return_Lk(tree);

      tree->c_lnL  = new_lnL_data;
      
      new_lnL_rate = RATES_Lk_Rates(tree);
	    
      ratio =
	(new_lnL_data + new_lnL_rate + log(new_mu)) -
	(cur_lnL_data + cur_lnL_rate + log(cur_mu));

/*       ratio = */
/*       	(new_lnL_rate + log(new_mu)) - */
/*       	(cur_lnL_rate + log(cur_mu)); */

      ratio = exp(ratio);	
      alpha = MIN(1.,ratio);
      
      u = Uni();
      
      if(u > alpha) /* Reject */
	{
	  tree->rates->nd_r[d->num] = cur_mu;
	  RATES_Update_Cur_Bl(tree);

	  if(tree->rates->lk_approx == NORMAL)
	    new_lnL_data = Dnorm_Multi_Given_InvCov_Det(tree->rates->u_cur_l,tree->rates->u_ml_l,tree->rates->invcov,tree->rates->covdet,2*tree->n_otu-3,YES);
	  else
	    new_lnL_data = Return_Lk(tree);

	  tree->c_lnL  = new_lnL_data;
	  
	  RATES_Lk_Rates(tree);

	  if((tree->mcmc->run > 10) && ((fabs(cur_lnL_data - tree->c_lnL) > 1.E-3) || (fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-0)))
	    {
	      printf("\n. Run=%d a=%d d=%d lexp=%f alpha=%f mu(d)=%f n(d)=%d mu(a)=%f n(a)=%d dt=(%f); cur_lnL_data = %f vs %f ; cur_lnL_rates = %f vs %f",
		     tree->mcmc->run,
		     d->anc->num,d->num,
		     tree->rates->lexp,
		     tree->rates->alpha,
		     tree->rates->nd_r[d->num],tree->rates->n_jps[d->num],
		     tree->rates->nd_r[d->anc->num],tree->rates->n_jps[d->anc->num],
		     fabs(tree->rates->nd_t[a->num] - tree->rates->nd_t[d->num]),
		     cur_lnL_data,tree->c_lnL,
		     cur_lnL_rate,tree->rates->c_lnL);
	      
	      PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
	      Warn_And_Exit("");
	    }
	  
	  if(fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-3)
	    {
	      printf("\n. WARNING: numerical precision issue detected (diff=%G run=%d). Reseting the likelihood.\n",cur_lnL_rate - tree->rates->c_lnL,tree->mcmc->run);

	      if(tree->rates->lk_approx == NORMAL)
		tree->c_lnL = Dnorm_Multi_Given_InvCov_Det(tree->rates->u_cur_l,tree->rates->u_ml_l,tree->rates->invcov,tree->rates->covdet,2*tree->n_otu-3,YES);
	      else
		tree->c_lnL = Return_Lk(tree);


	      if(tree->rates->lk_approx == NORMAL)
		tree->c_lnL = Dnorm_Multi_Given_InvCov_Det(tree->rates->u_cur_l,tree->rates->u_ml_l,tree->rates->invcov,tree->rates->covdet,2*tree->n_otu-3,YES);
	      else
		tree->c_lnL = Return_Lk(tree);

	      RATES_Lk_Rates(tree);
	    }
	}
      else
	{
	  tree->mcmc->acc_rates++;
	}
    }
      
  tree->mcmc->run++;
  MCMC_Print_Param(tree->mcmc,tree);

/*   if(d->tax) return; */
/*   else */
/*     { */
/*       For(i,3) */
/* 	{ */
/* 	  if((d->v[i] != a) && (d->b[i] != tree->e_root)) */
/* 	    { */
/* /\* 	      Update_P_Lk(tree,d->b[i],d); *\/ */
/* 	      MCMC_Rates_Pre(d,d->v[i],local,tree); */
/* 	    } */
/* 	} */
/* /\*       if(a != tree->n_root) { Update_P_Lk(tree,b,d); } *\/ */
/* /\*       else                  { Update_P_Lk(tree,tree->e_root,d); } *\/ */
/*     } */
}

/*********************************************************/

void MCMC_Times_Pre(node *a, node *d, int local, arbre *tree)
{
  phydbl u;
  phydbl t_min,t_max;
  phydbl cur_t, new_t;
  phydbl cur_lnL_times, new_lnL_times;
  phydbl cur_lnL_rate, new_lnL_rate;
  phydbl cur_lnL_data, new_lnL_data;
  phydbl ratio,alpha;
  int    i;
  phydbl t0,t1,t2,t3;
  node *v2,*v3, *buff_n;
  phydbl u0,u1,u2,u3;

  if(d->tax) return; /* Won't change time at tip */

  RATES_Record_Times(tree);
  RATES_Record_Rates(tree);

  cur_lnL_data  = tree->c_lnL;
  cur_t         = tree->rates->nd_t[d->num];
  cur_lnL_times = RATES_Yule(tree);
  cur_lnL_rate  = tree->rates->c_lnL;
  new_lnL_data  = cur_lnL_data;
  new_lnL_times = cur_lnL_times;
  new_lnL_rate  = cur_lnL_rate;

  buff_n = v2 = v3 = NULL;
  For(i,3)
    if((d->v[i] != a) && (d->b[i] != tree->e_root))
      {
	if(!v2) { v2 = d->v[i]; }
	else    { v3 = d->v[i]; }
      }
  
  t2 = tree->rates->nd_t[v2->num];
  t3 = tree->rates->nd_t[v3->num];

  if(t3 > t2)
    {
      buff_n = v2;
      v2     = v3;
      v3     = buff_n;
    }
  
  t0 = tree->rates->nd_t[a->num];
  t1 = tree->rates->nd_t[d->num];
  t2 = tree->rates->nd_t[v2->num];
  t3 = tree->rates->nd_t[v3->num];
  u0 = tree->rates->nd_r[a->num]  * tree->rates->clock_r;
  u1 = tree->rates->nd_r[d->num]  * tree->rates->clock_r;
  u2 = tree->rates->nd_r[v2->num] * tree->rates->clock_r;
  u3 = tree->rates->nd_r[v3->num] * tree->rates->clock_r;

  t_min = t0 + MAX((1./tree->rates->nu)*pow((u1-u0)/1.96,2),tree->rates->min_dt);
  t_max = MIN(t2 - MAX((1./tree->rates->nu)*pow((u1-u2)/1.96,2),tree->rates->min_dt), 
	      t3 - MAX((1./tree->rates->nu)*pow((u1-u3)/1.96,2),tree->rates->min_dt));

  t_min = MAX(t_min,tree->rates->t_prior_min[d->num]);
  t_max = MIN(t_max,tree->rates->t_prior_max[d->num]);

  if(fabs(t_min - t_max) < tree->rates->min_dt) return;
  
  tree->rates->t_prior[d->num] = Uni() * (t_max - t_min) + t_min;


  if(t_max < t_min)
    {
      PhyML_Printf("\n. WARNING: detected inconsistency in setting max and/or min time.");
      PhyML_Printf("\n. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
      PhyML_Printf("\n. %s %d",__FILE__,__LINE__);
      PhyML_Printf("\n. Has prior: %s",tree->rates->t_has_prior[d->num] ? "yes" : "no");
      PhyML_Printf("\n. Low=%f Up=%f",tree->rates->t_prior_min[d->num],tree->rates->t_prior_max[d->num]);
      PhyML_Printf("\n. NU=%f",tree->rates->nu);
      PhyML_Printf("\n. T0=%f T1=%f T2=%f T3=%f",t0,t1,t2,t3);
      PhyML_Printf("\n. U0=%f U1=%f U2=%f U3=%f",u0,u1,u2,u3);
      PhyML_Printf("\n. root_rate = %f",tree->rates->nd_r[a->num]);
      PhyML_Printf("\n. clock_rate = %f",tree->rates->clock_r);
      PhyML_Printf("\n. T_MAX=%f T_MIN=%f",t_max,t_min);
      PhyML_Printf("\n. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
      PhyML_Printf("\n");
      t_max = MIN(t2,t3);
      t_min = t0;
      Exit("\n");
    }


  u = Uni();    
  new_t = u*(t_max-t_min)+t_min;
/*   new_t = cur_t * exp(H_MCMC_RATES*(u-0.5)); */

  if(new_t < t_min)
    {
      new_t = t_max - fmod(fabs(new_t),fabs(t_max-t_min));
    }
  if(new_t > t_max)
    {
      new_t = t_min + fmod(fabs(new_t),fabs(t_max-t_min));
    }
  if(new_t < t_min || new_t > t_max)
    {
      PhyML_Printf("\n. Problem with setting new time.\n");
      PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
      Warn_And_Exit("");
    }


  tree->rates->nd_t[d->num] = new_t;
  
  if(local)
    {
      if(tree->rates->lk_approx == NORMAL)
	{
	  RATES_Update_Cur_Bl(tree);
	  new_lnL_data = Dnorm_Multi_Given_InvCov_Det(tree->rates->u_cur_l,tree->rates->u_ml_l,tree->rates->invcov,tree->rates->covdet,2*tree->n_otu-3,YES);
	}
      else
	{
	  new_lnL_data = Return_Lk(tree);
	}

      tree->c_lnL = new_lnL_data;            
/*       ratio = new_lnL_data - cur_lnL_data + log(fabs(new_t)) - log(fabs(cur_t)); */
      ratio = new_lnL_data - cur_lnL_data ;
      ratio = exp(ratio);
      alpha = MIN(1.,ratio);
      u = Uni();
      
      if(u > alpha) /* Reject */
	{
	  if(d->num == 33)
	    {
/*  	      printf("\n. REJECT %15f %15f %15f (%15f %15f)", */
/* 		     t_min,new_t,t_max, */
/* 		     new_lnL_data,cur_lnL_data); */

/* 	      phydbl *buff1,*buff2; */
/* 	      int i,size; */
/* 	      phydbl *xmmu; */
	      
/* 	      size = 2*tree->n_otu-3; */
/* 	      xmmu = (phydbl *)mCalloc(size,sizeof(phydbl)); */
	      
/* 	      For(i,size) xmmu[i] = tree->rates->u_cur_l[i] - tree->rates->u_ml_l[i]; */
	      
/* 	      buff1 = Matrix_Mult(xmmu,tree->rates->invcov,1,size,size,size); */
/* 	      buff2 = Matrix_Mult(buff1,xmmu,1,size,size,1); */
	      
/* 	      For(i,size) printf("\n. %15f %15f %15f %G %f",xmmu[i],buff1[i],tree->rates->invcov[i*size+4],tree->rates->cov[i*size+4],tree->t_edges[i]->l); */

/* 	      printf("\n. buff2[0] = %f",buff2[0]); */

/* 	      Free(xmmu); */
	    }

	  RATES_Reset_Times(tree);

	  if(tree->rates->lk_approx == NORMAL)
	    {
	      RATES_Update_Cur_Bl(tree);
	      new_lnL_data = Dnorm_Multi_Given_InvCov_Det(tree->rates->u_cur_l,tree->rates->u_ml_l,tree->rates->invcov,tree->rates->covdet,2*tree->n_otu-3,YES);
	    }
	  else
	    {
	      new_lnL_data = Return_Lk(tree);
	    }

	  tree->c_lnL = new_lnL_data;
	  
	  if((tree->mcmc->run > 10) && ((fabs(cur_lnL_data - tree->c_lnL) > 1.E-3) || (fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-0)))
	    {
	      printf("\n. Run=%d",tree->mcmc->run);
	      
	      printf("\n. lexp = %f alpha = %f",
		     tree->rates->lexp,
		     tree->rates->alpha);
	      	      
	      printf("\n. cur_t = %f ; cur_lnL_data = %f vs %f ; cur_lnL_rates = %f vs %f",
		     cur_t,
		     cur_lnL_data,tree->c_lnL,
		     cur_lnL_rate,tree->rates->c_lnL);
	      
	      PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
	      Warn_And_Exit("");
	    }
	  
	  if(fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-3)
	    {
	      printf("\n. WARNING: numerical precision issue detected (diff=%G). Reseting the likelihood.\n",cur_lnL_rate - tree->rates->c_lnL);
	      RATES_Lk_Rates(tree);
	    }
	}
      else
	{
	  RATES_Lk_Rates(tree); /* Rates likelihood needs to be updated here */
	  tree->mcmc->acc_times++;
	}
    }

  tree->mcmc->run++;
  MCMC_Print_Param(tree->mcmc,tree);
}

/*********************************************************/

void MCMC_Time_Root(arbre *tree)
{
  phydbl u;
  phydbl t_min,t_max;
  phydbl cur_t, new_t;
  phydbl cur_lnL_times, new_lnL_times;
  phydbl cur_lnL_rate, new_lnL_rate;
  phydbl cur_lnL_data, new_lnL_data;
  phydbl ratio,alpha;
  phydbl t0,t2,t3;
  node *v2,*v3, *buff_n;
  phydbl u0,u2,u3;
  node *root;


  RATES_Record_Times(tree);
  
  root = tree->n_root;

  cur_lnL_data  = tree->c_lnL;
  cur_t         = tree->rates->nd_t[root->num];
  cur_lnL_times = RATES_Yule(tree);
  cur_lnL_rate  = tree->rates->c_lnL;
  new_lnL_data  = cur_lnL_data;
  new_lnL_times = cur_lnL_times;
  new_lnL_rate  = cur_lnL_rate;

  v2 = root->v[0];
  v3 = root->v[1];

  t2 = tree->rates->nd_t[v2->num];
  t3 = tree->rates->nd_t[v3->num];

  if(t3 > t2)
    {
      buff_n = v2;
      v2     = v3;
      v3     = buff_n;
    }
  
  t0 = tree->rates->nd_t[root->num];
  t2 = tree->rates->nd_t[v2->num];
  t3 = tree->rates->nd_t[v3->num];
  u2 = tree->rates->nd_r[v2->num] * tree->rates->clock_r;
  u3 = tree->rates->nd_r[v3->num] * tree->rates->clock_r;
  u0 = 1.0  * tree->rates->clock_r;

  t_min = tree->rates->t_prior_min[root->num];
  t_max = MIN(t2 - (1./tree->rates->nu)*pow((u0-u2)/1.96,2), t3 - (1./tree->rates->nu)*pow((u0-u3)/1.96,2));

  t_min = MAX(t_min,tree->rates->t_prior_min[root->num]);
  t_max = MIN(t_max,tree->rates->t_prior_max[root->num]);
  
  if(fabs(t_min - t_max) < tree->rates->min_dt) return;

  tree->rates->t_prior[root->num] = Uni() * (t_max - t_min) + t_min;
  
  if(t_max < t_min)
    {
      PhyML_Printf("\n. WARNING: detected inconsistency in setting max and/or min time.");
      PhyML_Printf("\n. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
      PhyML_Printf("\n. %s %d",__FILE__,__LINE__);
      PhyML_Printf("\n. NU=%f",tree->rates->nu);
      PhyML_Printf("\n. T0=%f T2=%f T3=%f",t0,t2,t3);
      PhyML_Printf("\n. U0=%f U2=%f U3=%f",u0,u2,u3);
      PhyML_Printf("\n. T_MAX=%f T_MIN=%f",t_max,t_min);
      PhyML_Printf("\n. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
      PhyML_Printf("\n");
      t_max = MIN(t2,t3);
      t_min = tree->rates->t_prior_min[root->num];
/*       PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__); */
/*       Exit("\n"); */
    }
  
  u = Uni();    
/*   new_t = u*(t_max-t_min)+t_min;   */
  new_t = cur_t * exp(H_MCMC_RATES*(u-0.5));

  if(new_t < t_min)
    {
      new_t = t_max - fmod(fabs(new_t),fabs(t_max-t_min));
    }
  if(new_t > t_max)
    {
      new_t = t_min + fmod(fabs(new_t),fabs(t_max-t_min));
    }
  if(new_t < t_min || new_t > t_max)
    {
      PhyML_Printf("\n. Problem with setting new time.\n");
      PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
      Warn_And_Exit("");
    }

  tree->rates->nd_t[root->num] = new_t;
  
  if(tree->rates->lk_approx == NORMAL)
    {
      RATES_Update_Cur_Bl(tree);
      new_lnL_data = Dnorm_Multi_Given_InvCov_Det(tree->rates->u_cur_l,tree->rates->u_ml_l,tree->rates->invcov,tree->rates->covdet,2*tree->n_otu-3,YES);
    }
  else
    {
      new_lnL_data = Return_Lk(tree);
    }
  
  tree->c_lnL = new_lnL_data;            
  ratio = new_lnL_data - cur_lnL_data + log(fabs(new_t)) - log(fabs(cur_t));      
  ratio = exp(ratio);
  alpha = MIN(1.,ratio);
  u = Uni();
  
  
  if(u > alpha) /* Reject */
    {
      RATES_Reset_Times(tree);
      
      if(tree->rates->lk_approx == NORMAL)
	{
	  RATES_Update_Cur_Bl(tree);
	  new_lnL_data = Dnorm_Multi_Given_InvCov_Det(tree->rates->u_cur_l,tree->rates->u_ml_l,tree->rates->invcov,tree->rates->covdet,2*tree->n_otu-3,YES);
	}
      else
	{
	  new_lnL_data = Return_Lk(tree);
	}
      
      tree->c_lnL = new_lnL_data;
      
      if((tree->mcmc->run > 10) && ((fabs(cur_lnL_data - tree->c_lnL) > 1.E-3) || (fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-0)))
	{
	  printf("\n. Run=%d",tree->mcmc->run);
	  
	  printf("\n. lexp = %f alpha = %f",
		 tree->rates->lexp,
		 tree->rates->alpha);
	  
	  printf("\n. cur_t = %f ; cur_lnL_data = %f vs %f ; cur_lnL_rates = %f vs %f",
		 cur_t,
		 cur_lnL_data,tree->c_lnL,
		 cur_lnL_rate,tree->rates->c_lnL);
	  
	  PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
	  Warn_And_Exit("");
	}
      
      if(fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-3)
	{
	  printf("\n. WARNING: numerical precision issue detected (diff=%G). Reseting the likelihood.\n",cur_lnL_rate - tree->rates->c_lnL);
	  RATES_Lk_Rates(tree);
	}
    }
  else
    {
      RATES_Lk_Rates(tree); /* Rates likelihood needs to be updated here because variances of rates have changed */
      tree->mcmc->acc_times++;
    }
    
  tree->mcmc->run++;
  MCMC_Print_Param(tree->mcmc,tree);
}

/*********************************************************/

void MCMC_Jumps_Local(arbre *tree)
{
  int local;
  local = 1;
  MCMC_Jumps_Pre(tree->n_root,tree->n_root->v[0],local,tree);
  MCMC_Jumps_Pre(tree->n_root,tree->n_root->v[1],local,tree);
}

/*********************************************************/

void MCMC_Jumps_Pre(node *a, node *d, int local, arbre *tree)
{
  phydbl u;
  phydbl new_lnL_rate, cur_lnL_rate;
/*   phydbl new_lnL_jps, cur_lnL_jps; */
  phydbl ratio, alpha;
  int new_jps, cur_jps;
  int i;

  cur_jps      = tree->rates->n_jps[d->num];
  cur_lnL_rate = tree->rates->c_lnL;
/*   cur_lnL_jps  = RATES_Lk_Jumps(tree); */
  
  u = Uni();

  new_jps = cur_jps + (int)((u-0.5)*4.);

  if(local && new_jps > -1 && new_jps < 30)
    {
      tree->rates->n_jps[d->num] = new_jps;

      new_lnL_rate = RATES_Lk_Rates(tree);
/*       new_lnL_jps  = RATES_Lk_Jumps(tree); */

      ratio = (new_lnL_rate) - (cur_lnL_rate);
/*       ratio = (new_lnL_rate + new_lnL_jps) - (cur_lnL_rate + cur_lnL_jps); */
      ratio = exp(ratio);
      alpha = MIN(1.,ratio);
      
      u = Uni();
      
      if(u > alpha) /* Reject */
	{
	  tree->rates->n_jps[d->num] = cur_jps;

	  RATES_Lk_Rates(tree);
	  
	  if(fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-0)
	    {
	      printf("\n. lexp=%f alpha=%f dt=(%f); cur_lnL_rates = %f vs %f",
		     tree->rates->lexp,
		     tree->rates->alpha,
		     fabs(tree->rates->nd_t[a->num] - tree->rates->nd_t[d->num]),
		     cur_lnL_rate,tree->rates->c_lnL);
	      
	      PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
	      Warn_And_Exit("");
	    }
	  
	  if(fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-3)
	    {
	      printf("\n. WARNING: numerical precision issue detected (diff=%G). Reseting the likelihood.\n",cur_lnL_rate - tree->rates->c_lnL);
	      RATES_Lk_Rates(tree);
	    }
	}
    }

  tree->mcmc->run++;
  MCMC_Print_Param(tree->mcmc,tree);

  if(d->tax) return;
  else
    {
      For(i,3)
	{
	  if((d->v[i] != a) && (d->b[i] != tree->e_root))
	    {
	      MCMC_Jumps_Pre(d,d->v[i],local,tree);
	    }
	}
    }
}

/*********************************************************/

void MCMC_Stick_Rates_Pre(node *a, node *d, arbre *tree)
{
  phydbl u;
  phydbl new_lnL_data, cur_lnL_data, new_lnL_rate, cur_lnL_rate;
  phydbl dta,dtd;
  phydbl ratio, alpha, hr;
  phydbl new_mu, cur_mu;
  int i;
  edge *b;

  b = NULL;

  if(a != tree->n_root)
    {      
      cur_mu       = tree->rates->nd_r[d->num];
      cur_lnL_data = tree->c_lnL;
      cur_lnL_rate = tree->rates->c_lnL;
      
      dta = fabs(tree->rates->nd_t[a->num] - tree->rates->nd_t[a->anc->num]);
      dtd = fabs(tree->rates->nd_t[d->num] - tree->rates->nd_t[d->anc->num]);

      For(i,3) if(d->v[i] == a) {b = d->b[i]; break;}

      u = Uni();
      
      if(u < exp(-tree->rates->lexp * (dta+dtd)))
	{
	  new_mu = tree->rates->nd_r[a->num];
      
/* 	  new_lnL_rate = RATES_Lk_Rates(tree); */
	  new_lnL_rate = RATES_Lk_Change_One_Rate(d,new_mu,tree);
	  new_lnL_data = Lk_At_Given_Edge(b,tree);
	  
	  hr = (1. - exp(-tree->rates->lexp * (dta+dtd))) / exp(-tree->rates->lexp * (dta+dtd));
	  	  
	  ratio =
	    (new_lnL_data + new_lnL_rate) -
	    (cur_lnL_data + cur_lnL_rate) +
	    log(hr);
	  
	  ratio = exp(ratio);	
	  alpha = MIN(1.,ratio);
	  
	  u = Uni();
	  
	  if(u > alpha) /* Reject */
	    {
/* 	      RATES_Lk_Rates(tree); */
	      RATES_Lk_Change_One_Rate(d,cur_mu,tree);
	      Lk_At_Given_Edge(b,tree);
	      
	      if((fabs(cur_lnL_data - tree->c_lnL) > 1.E-3) || (fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-0))
		{
		  printf("\n. lexp=%f alpha=%f b->l = (%f) dt=(%f); cur_lnL_data = %f vs %f ; cur_lnL_rates = %f vs %f",
			 tree->rates->lexp,
			 tree->rates->alpha,
			 b->l,
			 fabs(tree->rates->nd_t[a->num] - tree->rates->nd_t[d->num]),
			 cur_lnL_data,tree->c_lnL,
			 cur_lnL_rate,tree->rates->c_lnL);
		  
		  PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
		  Warn_And_Exit("");
		}
	      
	      if(fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-3)
		{
		  printf("\n. WARNING: numerical precision issue detected (diff=%G). Reseting the likelihood.\n",cur_lnL_rate - tree->rates->c_lnL);
		  RATES_Lk_Rates(tree);
		  Lk(tree);
		}
	    }
	}
    }

  tree->mcmc->run++;
  MCMC_Print_Param(tree->mcmc,tree);

  if(d->tax) return;
  else
    {
      For(i,3)
	{
	  if((d->v[i] != a) && (d->b[i] != tree->e_root))
	    {
	      Update_P_Lk(tree,d->b[i],d);
	      MCMC_Stick_Rates_Pre(d,d->v[i],tree);
	    }
	}
      if(a != tree->n_root) { Update_P_Lk(tree,b,d); }
      else                  { Update_P_Lk(tree,tree->e_root,d); }
    }
}

/*********************************************************/


void MCMC_Print_Param(tmcmc *mcmc, arbre *tree)
{
  int i;
  FILE *fp;

  fp = mcmc->out_fp_stats;
  

  if(!(mcmc->run%mcmc->sample_interval)) 
    {
      
      if(tree->mcmc->run == 0)
	{

	  PhyML_Fprintf(fp,"\n");
	  PhyML_Fprintf(fp,"Run\t");
/* 	  PhyML_Fprintf(fp,"TreeSize\t"); */
	  PhyML_Fprintf(fp,"LnLSeq\t");
	  PhyML_Fprintf(fp,"LnLRate\t");
/* 	  PhyML_Fprintf(fp,"RootPos[%f]\t",tree->n_root_pos); */
	  PhyML_Fprintf(fp,"Nu\t");
	  PhyML_Fprintf(fp,"Clock\t");

	  if(fp != stdout) 
	    {
	      for(i=tree->n_otu;i<2*tree->n_otu-1;i++)
		{
		  if(i == tree->n_root->num)
		    {
		      PhyML_Fprintf(fp,"XXT%d[%f]\t",i,tree->rates->true_t[i]);
		    }
		  else
		    {
		      phydbl min_t = +1E+5;
		      int j;
		      
		      For(j,3) 
			if(tree->noeud[i]->v[j] != tree->noeud[i]->anc && tree->noeud[i]->b[j] != tree->e_root)
			  {
			    if(tree->rates->true_t[tree->noeud[i]->v[j]->num] < min_t)
			      {
				min_t = tree->rates->true_t[tree->noeud[i]->v[j]->num];
			      }
			  }

		      if(tree->rates->t_has_prior[i])
			{
			  PhyML_Fprintf(fp,"**T%d[%.1f<%.1f<%.1f]\t",i,
				  -100.,
				  tree->rates->true_t[i],
				  min_t);
			}
		      else
			{
			  PhyML_Fprintf(fp,"  T%d[%.1f<%.1f<%.1f]\t",
				  i,
				  tree->rates->true_t[tree->noeud[i]->anc->num],
				  tree->rates->true_t[i],
				  min_t);
			}
		    }
		}
/* 	      for(i=tree->n_otu;i<2*tree->n_otu-1;i++) PhyML_Fprintf(fp," pT%d\t",i); */
	    }

/* 	  if(fp != stdout) */
/* 	    for(i=0;i<2*tree->n_otu-2;i++)  */
/* 	      if( */
/* 		 (tree->noeud[i] == tree->n_root->v[0] && tree->noeud[i]->tax) || */
/* 		 (tree->noeud[i] == tree->n_root->v[1] && tree->noeud[i]->tax) */
/* 		 ) */
/* 		PhyML_Fprintf(fp,"00R%d[%f]\t",i,tree->rates->true_r[i]); */
/* 	      else if((tree->noeud[i] == tree->n_root->v[0]) || (tree->noeud[i] == tree->n_root->v[1])) */
/* 		PhyML_Fprintf(fp,"11R%d[%f]\t",i,tree->rates->true_r[i]); */
/* 	      else PhyML_Fprintf(fp,"  R%d[%f]\t",i,tree->rates->true_r[i]); */

/* 	  if(fp != stdout)  */
/* 	    for(i=0;i<2*tree->n_otu-3;i++)  */
/* 	      { */
/* 		if(tree->t_edges[i] == tree->e_root) PhyML_Fprintf(fp,"**L%d[%f,%G]\t",i,tree->rates->u_ml_l[i],tree->rates->cov[i*(2*tree->n_otu-3)+i]); */
/* 		else PhyML_Fprintf(fp,"  L%d[%f,%G]\t",i,tree->rates->u_ml_l[i],tree->rates->cov[i*(2*tree->n_otu-3)+i]); */
/* 	      }	   */

/* 	  if(fp != stdout) PhyML_Fprintf(fp,"AccRate\t"); */

	  PhyML_Fprintf(mcmc->out_fp_trees,"#NEXUS\n");
	  PhyML_Fprintf(mcmc->out_fp_trees,"BEGIN TREES;\n");

	}


      PhyML_Fprintf(fp,"\n");
      PhyML_Fprintf(fp,"%6d\t",tree->mcmc->run);
/*       PhyML_Fprintf(fp,"%4.2f\t",RATES_Check_Mean_Rates(tree)); */
/*       PhyML_Fprintf(fp,"%4.2f\t",Get_Tree_Size(tree)); */

      PhyML_Fprintf(fp,"%15lf\t",tree->c_lnL);

      PhyML_Fprintf(fp,"%15lf\t",tree->rates->c_lnL);

/*       PhyML_Fprintf(fp,"%15lf\t",tree->rates->cur_l[tree->n_root->v[0]->num] / tree->rates->u_cur_l[tree->e_root->num]-tree->n_root_pos); */
      PhyML_Fprintf(fp,"%15lf\t",tree->rates->nu);
      PhyML_Fprintf(fp,"%15lf\t",tree->rates->clock_r);
/*       if(fp != stdout) for(i=tree->n_otu;i<2*tree->n_otu-1;i++) PhyML_Fprintf(fp,"%8f\t",tree->rates->nd_t[i] - tree->rates->true_t[i]); */
/*       if(fp != stdout) for(i=tree->n_otu;i<2*tree->n_otu-1;i++) PhyML_Fprintf(fp,"%8f\t",tree->rates->t_prior[i] - tree->rates->true_t[i]); */
      if(fp != stdout) for(i=tree->n_otu;i<2*tree->n_otu-1;i++) PhyML_Fprintf(fp,"%8f\t",tree->rates->nd_t[i]);
/*       if(fp != stdout) for(i=tree->n_otu;i<2*tree->n_otu-1;i++) PhyML_Fprintf(fp,"%8f\t",tree->rates->t_prior[i]); */
/*       if(fp != stdout) for(i=0;i<2*tree->n_otu-2;i++) PhyML_Fprintf(fp,"%8f\t",tree->rates->nd_r[i]); */
/*       if(fp != stdout) for(i=0;i<2*tree->n_otu-3;i++) PhyML_Fprintf(fp,"%8f\t",tree->rates->u_cur_l[i]); */
/*       if(fp != stdout)  */
/* 	{ */
/* 	  if(tree->mcmc->run) */
/* 	    PhyML_Fprintf(fp,"%8f\t",(phydbl)tree->mcmc->acc_rates/tree->mcmc->run); */
/* 	  else */
/* 	    PhyML_Fprintf(fp,"%8f\t",0.0); */
/* 	} */
      fflush(NULL);


      // TREES
      char *s;     
      Branch_Lengths_To_Time_Lengths(tree);
      s = Write_Tree(tree);
      PhyML_Fprintf(mcmc->out_fp_trees,"TREE %8d [%f] = [&R] %s\n",mcmc->run,tree->c_lnL,s);
      Free(s);
      RATES_Update_Cur_Bl(tree);


    }
}

/*********************************************************/

tmcmc *MCMC_Make_MCMC_Struct(arbre *tree)
{
  tmcmc *mcmc;
  
  mcmc               = (tmcmc *)mCalloc(1,sizeof(tmcmc));
  mcmc->dt_prop      = (phydbl *)mCalloc(tree->n_otu-2,sizeof(phydbl));
  mcmc->p_no_jump    = (phydbl *)mCalloc(2*tree->n_otu-3,sizeof(phydbl));
  mcmc->t_rate_jumps = (phydbl *)mCalloc(10*tree->n_otu,sizeof(phydbl));
  mcmc->t_rank       = (int *)mCalloc(tree->n_otu-1,sizeof(int));
  mcmc->r_path       = (phydbl *)mCalloc(tree->n_otu-2,sizeof(phydbl));
  mcmc->out_filename = (char *)mCalloc(T_MAX_FILE,sizeof(char));
  return(mcmc);
}

/*********************************************************/

void MCMC_Free_MCMC(tmcmc *mcmc)
{
  Free(mcmc->dt_prop);
  Free(mcmc->p_no_jump);
  Free(mcmc->t_rate_jumps);
  Free(mcmc->t_rank);
  Free(mcmc->r_path);
  Free(mcmc->out_filename);
  Free(mcmc);
}

/*********************************************************/

void MCMC_Init_MCMC_Struct(char *filename, tmcmc *mcmc, arbre *tree)
{
  int pid;

  mcmc->acc_lexp        = 0;
  mcmc->acc_rates       = 0;
  mcmc->acc_times       = 0;
  mcmc->acc_nu          = 0;
  mcmc->run             = 0;
  mcmc->sample_interval = 100;  
  mcmc->n_rate_jumps    = 0;
  mcmc->n_tot_run       = 1.E+6;

  if(filename)
    {
      strcpy(mcmc->out_filename,filename);
      pid = getpid();
      sprintf(mcmc->out_filename+strlen(mcmc->out_filename),".%d",pid);
    }

  if(filename) 
    {
      char *s;
      s = (char *)mCalloc(10,sizeof(char));
      strcpy(s,".trees");
      tree->mcmc->out_fp_stats = fopen(tree->mcmc->out_filename,"w");
      tree->mcmc->out_fp_trees = fopen(strcat(tree->mcmc->out_filename,s),"w");
      Free(s);
    }
  else 
    {
      tree->mcmc->out_fp_stats = stderr;
      tree->mcmc->out_fp_trees = stderr;
    }
}

/*********************************************************/

void MCMC_Randomize_Branch_Lengths(arbre *tree)
{
  int i;
  phydbl u;

  For(i,2*tree->n_otu-3)
    {
      if(tree->t_edges[i] != tree->e_root)
	{
	  u = Uni();
	  tree->t_edges[i]->l *= -log(u);
	}
      else
	{
	  printf("\n. Didn't randomize root edge.");
	}
    }
}

/*********************************************************/

void MCMC_Randomize_Rates(arbre *tree)
{
  int i;
  phydbl u;

  For(i,2*tree->n_otu-2) tree->rates->nd_r[i] = 1.0;

  For(i,2*tree->n_otu-2)
    {
      u = Uni();
      u = u * (1.2-0.8) + 0.8;
      tree->rates->nd_r[i] = u;
      if(tree->rates->nd_r[i] < tree->rates->min_rate) tree->rates->nd_r[i] = tree->rates->min_rate; 
      if(tree->rates->nd_r[i] > tree->rates->max_rate) tree->rates->nd_r[i] = tree->rates->max_rate; 
    }
}

/*********************************************************/

void MCMC_Randomize_Jumps(arbre *tree)
{
  int i;
  phydbl u;

  For(i,2*tree->n_otu-2)
    {
      u = Uni();
      u *= 4.;
      tree->rates->n_jps[i] = (int)(u)+1;
      if(tree->rates->n_jps[i] > 10) tree->rates->n_jps[i] = 10;
    }

}
/*********************************************************/

void MCMC_Randomize_Lexp(arbre *tree)
{
  phydbl u;

  tree->rates->lexp = -1.0;
  do
    {
      u = Uni();
      tree->rates->lexp = -log(u);
    }
  while((tree->rates->lexp < 1.E-5) || (tree->rates->lexp > 2.0));
}

/*********************************************************/

void MCMC_Randomize_Nu(arbre *tree)
{
  phydbl u;
  do
    {
      u = Uni();
      tree->rates->nu = -log(u);
    }
  while((tree->rates->nu < 1.E-5) || (tree->rates->nu > 1.E-1));
}

/*********************************************************/

void MCMC_Randomize_Clock_Rate(arbre *tree)
{
  phydbl u;
  u = Uni();
  tree->rates->clock_r = u * (tree->rates->max_clock - tree->rates->min_clock) + tree->rates->min_clock;
}

/*********************************************************/

void MCMC_Randomize_Alpha(arbre *tree)
{
  phydbl u;

  u = Uni();
  tree->rates->alpha = u*6.0+1.0;
}

/*********************************************************/

void MCMC_Randomize_Node_Times(arbre *tree)
{
  phydbl t_sup, t_inf;
  phydbl u;
  phydbl min_prior;
  int i;

  min_prior = 0.0;
  
  MCMC_Randomize_Node_Times_Bottom_Up(tree->n_root,tree->n_root->v[0],tree);
  MCMC_Randomize_Node_Times_Bottom_Up(tree->n_root,tree->n_root->v[1],tree);

  if(tree->rates->t_has_prior[tree->n_root->num])
    {
      t_inf = tree->rates->t_prior_min[tree->n_root->num];
      t_sup = tree->rates->t_prior_max[tree->n_root->num];
      
      u = Uni();
      u *= (t_sup - t_inf);
      u += t_inf;
      
      tree->rates->nd_t[tree->n_root->num] = u;
      PhyML_Printf("\n. Root time set to %f (t_inf=%f t_sup=%f)",tree->rates->nd_t[tree->n_root->num],t_inf,t_sup);
    }
  else
    {
      For(i,2*tree->n_otu-2)
	{
	  if(tree->rates->t_has_prior[i] && tree->rates->t_prior_min[i] < min_prior)
	    {
	      min_prior = tree->rates->t_prior_min[i];
	    }
	}
      tree->rates->nd_t[tree->n_root->num] = 10. * min_prior;
    }
  

  MCMC_Randomize_Node_Times_Top_Down(tree->n_root,tree->n_root->v[0],tree);
  MCMC_Randomize_Node_Times_Top_Down(tree->n_root,tree->n_root->v[1],tree);

  For(i,2*tree->n_otu-1)
    {
      if(!tree->rates->t_has_prior[i])
	{
	  tree->rates->t_prior_min[i] = 1.E+2 * tree->rates->nd_t[tree->n_root->num];
	  tree->rates->t_prior_max[i] = 0.0;
	}
    }  

  For(i,2*tree->n_otu-1)
    {
      PhyML_Printf("\n. %f",tree->rates->nd_t[i]);
    }
}

/*********************************************************/

void MCMC_Randomize_Node_Times_Bottom_Up(node *a, node *d, arbre *tree)
{
  int i;
  phydbl t_inf,t_sup;
  phydbl u;

  if(d->tax) 
    {
      if(tree->rates->t_has_prior[d->num])
	{
	  t_inf = tree->rates->t_prior_min[d->num];
	  t_sup = tree->rates->t_prior_max[d->num];
	  
	  u = Uni();
	  u *= (t_sup - t_inf);
	  u += t_inf;
	  
	  tree->rates->nd_t[d->num] = u;	  
	}
      return;
    }
  else 
    {
      node *v1, *v2; /* the two sons of d */

      For(i,3)
	{
	  if((d->v[i] != a) && (d->b[i] != tree->e_root))
	    {
	      MCMC_Randomize_Node_Times_Bottom_Up(d,d->v[i],tree);	      
	    }
	}
      
      v1 = v2 = NULL;
      For(i,3) if((d->v[i] != a) && (d->b[i] != tree->e_root)) 
	{
	  if(!v1) v1 = d->v[i]; 
	  else    v2 = d->v[i];
	}
      
      if(tree->rates->t_has_prior[d->num])
	{
	  t_sup = MIN(tree->rates->t_prior_max[d->num],
		      MIN(tree->rates->t_prior_max[v1->num],tree->rates->t_prior_max[v2->num]));

	  tree->rates->t_prior_max[d->num] = t_sup;
	}
      else
	{
	  tree->rates->t_prior_max[d->num] = 
	    MIN(tree->rates->t_prior_max[v1->num],tree->rates->t_prior_max[v2->num]);
	}
    }
}

/*********************************************************/

void MCMC_Randomize_Node_Times_Top_Down(node *a, node *d, arbre *tree)
{
  if(d->tax) return;
  else
    {
      int i;      
      node *v1, *v2; /* the two sons of d */
      phydbl u;
      phydbl t_inf, t_sup;

      
      v1 = v2 = NULL;
      For(i,3) if((d->v[i] != a) && (d->b[i] != tree->e_root)) 
	{
	  if(!v1) v1 = d->v[i]; 
	  else    v2 = d->v[i];
	}

      t_inf = MAX(tree->rates->nd_t[a->num],tree->rates->t_prior_min[d->num]);
      t_sup = tree->rates->t_prior_max[d->num];
	  	  
      if(t_inf > t_sup)
	{
	  PhyML_Printf("\n. t_inf=%f t_sup=%f",t_inf,t_sup);
	  PhyML_Printf("\n. Inconsistency in the prior settings detected at node %d",d->num);
	  PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
	  Warn_And_Exit("\n");
	}
	  
      u = Uni();
      u *= (t_sup - t_inf);
      u += t_inf;
      
      tree->rates->nd_t[d->num] = u;
      

      For(i,3)
	{
	  if((d->v[i] != a) && (d->b[i] != tree->e_root))
	    {
	      MCMC_Randomize_Node_Times_Top_Down(d,d->v[i],tree);
	    }
	}
    }
}

/*********************************************************/

void MCMC_Mixing_Step(arbre *tree)
{
  phydbl new_lnL_time, new_lnL_rate, new_lnL_data;
  phydbl cur_lnL_time, cur_lnL_rate, cur_lnL_data;
  phydbl ratio,alpha,u,hr;
  int i;
  phydbl multiplier;

  cur_lnL_rate = tree->rates->c_lnL;
  cur_lnL_time = RATES_Yule(tree);
  cur_lnL_data = tree->c_lnL;
  
  new_lnL_data = cur_lnL_data;

  RATES_Record_Times(tree);

  u = Uni();
  multiplier = 1.0*exp(u-0.5);

  For(i,2*tree->n_otu-1) tree->rates->nd_t[i] *= multiplier;
  tree->rates->clock_r /= multiplier;
  
  RATES_Update_Cur_Bl(tree);
  
  hr = pow(multiplier,-(tree->n_otu-1));
  
  new_lnL_rate = RATES_Lk_Rates(tree);
  new_lnL_time = RATES_Yule(tree);
  /* new_lnL_data = Return_Lk(tree); */
  
  ratio = (new_lnL_rate + new_lnL_time) - (cur_lnL_rate + cur_lnL_time) + log(hr);
  ratio = exp(ratio);
  alpha = MIN(1.,ratio);
  
  u = Uni();
  
  if(u > alpha) /* Reject */
    {
      RATES_Reset_Times(tree);
      tree->rates->clock_r *= multiplier;

      RATES_Lk_Rates(tree);
      
      if(fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-0)
	{
	  printf("\n. lexp=%f alpha=%f ; cur_lnL_rates = %f vs %f",
		 tree->rates->lexp,
		 tree->rates->alpha,
		 cur_lnL_rate,tree->rates->c_lnL);

	  PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
	  Warn_And_Exit("");
	}

      if(fabs(cur_lnL_rate - tree->rates->c_lnL) > 1.E-3)
	{
	  printf("\n. WARNING: numerical precision issue detected (diff=%G run=%d). Reseting the likelihood.\n",cur_lnL_rate - tree->rates->c_lnL,tree->mcmc->run);
	  RATES_Lk_Rates(tree);
	  Lk(tree);
	}
    }
  else
    {
/*       printf("\n. Accept global times"); */
    }
}


/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/