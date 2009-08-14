/*

PhyML:  a program that  computes maximum likelihood phylogenies from
DNA or AA homologous sequences.

Copyright (C) Stephane Guindon. Oct 2003 onward.

All parts of the source except where indicated are distributed under
the GNU public licence. See http://www.opensource.org for details.

*/

#ifndef MCMC_H
#define MCMC_H

#include "spr.h"
#include "utilities.h"
#include "lk.h"
#include "optimiz.h"
#include "bionj.h"
#include "models.h"
#include "free.h"
#include "help.h"
#include "simu.h"
#include "eigen.h"
#include "pars.h"
#include "alrt.h"
#include "mc.h"
#include "m4.h"
#include "draw.h"
#include "rates.h"
#include "stats.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void MCMC_Lexp(t_tree *tree);
void MCMC_Print_Param(tmcmc *mcmc, t_tree *tree);
tmcmc *MCMC_Make_MCMC_Struct();
void MCMC_Free_MCMC(tmcmc *mcmc);
void MCMC_Init_MCMC_Struct(char *filename, tmcmc *mcmc, t_tree *tree);
void MCMC(t_tree *tree);
void MCMC_Alpha(t_tree *tree);
void MCMC_Randomize_Branch_Lengths(t_tree *tree);
void MCMC_Randomize_Node_Times(t_tree *tree);
void MCMC_Randomize_Node_Times_Pre(t_node *a, t_node *d, t_tree *tree);
void MCMC_Randomize_Lexp(t_tree *tree);
void MCMC_Randomize_Jumps(t_tree *tree);
void MCMC_Randomize_Alpha(t_tree *tree);
void MCMC_One_Node_Time(t_node *d, t_tree *tree);
void MCMC_One_Rate(t_node *a, t_node *d, t_tree *tree);
void MCMC_No_Change(t_tree *tree);
void MCMC_Nu(t_tree *tree);
void MCMC_Randomize_Nu(t_tree *tree);
t_node *MCMC_Select_Random_Node_Pair(phydbl t_sup, t_tree *tree);
void MCMC_Modify_Rates(t_tree *tree);
void MCMC_Modify_Subtree_Rate(t_node *a, t_node *d, phydbl new_rate, t_tree *tree);
void MCMC_Randomize_Rates(t_tree *tree);
void MCMC_Stick_Rates(t_tree *tree);
void MCMC_Stick_Rates_Pre(t_node *a, t_node *d, t_tree *tree);
void MCMC_Times_Global(t_tree *tree);
void MCMC_Times_Local(t_tree *tree);
void MCMC_Times_Pre(t_node *a, t_node *d, int local, t_tree *tree);
void MCMC_Rates_Global(t_tree *tree);
void MCMC_Rates_Local(t_tree *tree);
void MCMC_Rates_Pre(t_node *a, t_node *d, t_tree *tree);
void MCMC_Mixing_Step(t_tree *tree);
void MCMC_Jumps_Local(t_tree *tree);
void MCMC_Jumps_Pre(t_node *a, t_node *d, int local, t_tree *tree);
void MCMC_Randomize_Clock_Rate(t_tree *tree);
void MCMC_Clock_Rate(t_tree *tree);
void MCMC_Time_Root(t_tree *tree);
void MCMC_Randomize_Node_Times_Bottom_Up(t_node *a, t_node *d, t_tree *tree);
void MCMC_Randomize_Node_Times_Top_Down(t_node *a, t_node *d, t_tree *tree);
void MCMC_Randomize_Rates_Pre(t_node *a, t_node *d, t_tree *tree);
void MCMC_Print_Means(tmcmc *mcmc, t_tree *tree);
void MCMC_Print_Last(tmcmc *mcmc, t_tree *tree);
void MCMC_Close_MCMC(tmcmc *mcmc);
void MCMC_Rates_Global(t_tree *tree);



#endif