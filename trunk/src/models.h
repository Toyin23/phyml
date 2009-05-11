/*

PHYML :  a program that  computes maximum likelihood  phylogenies from
DNA or AA homologous sequences 

Copyright (C) Stephane Guindon. Oct 2003 onward

All parts of  the source except where indicated  are distributed under
the GNU public licence.  See http://www.opensource.org for details.

*/

#ifndef MODELS_H
#define MODELS_H

#include "utilities.h"
#include "eigen.h"
#include "free.h"
#include "stats.h"

void  PMat(phydbl l, model *mod, int pos, double *Pij);
void  PMat_K80(phydbl l,phydbl kappa, int pos, double *Pij);
void  PMat_TN93(phydbl l, model *mod, int pos, double *Pij);
void  PMat_Empirical(phydbl l, model *mod, int pos, double *Pij);
void PMat_Zero_Br_Len(model  *mod, int pos, double *Pij);
void PMat_Gamma(phydbl l, model *mod, int pos, double *Pij);

int GetDaa (phydbl *daa, phydbl *pi, char *file_name);
void Init_Model(allseq *data, model *mod);
void Update_Qmat_GTR(double *rr, phydbl *rr_val, int *rr_num, double *pi, double *qmat);
void Update_Qmat_HKY(double kappa, double *pi, double *qmat);
void Update_Qmat_Generic(double *rr, double *pi, int ns, double *qmat);
void Translate_Custom_Mod_String(model *mod);
void Set_Model_Parameters(model *mod);
phydbl GTR_Dist(phydbl *F, phydbl alpha, eigen *eigen_struct);
phydbl General_Dist(phydbl *F, model *mod, eigen *eigen_struct);

int Init_Qmat_WAG(double *daa, phydbl *pi);
int Init_Qmat_Dayhoff(double *daa, phydbl *pi);
int Init_Qmat_JTT(double *daa, phydbl *pi);
int Init_Qmat_RtREV(double *daa, phydbl *pi);
int Init_Qmat_CpREV(double *daa, phydbl *pi);
int Init_Qmat_VT(double *daa, phydbl *pi);
int Init_Qmat_Blosum62(double *daa, phydbl *pi);
int Init_Qmat_MtMam(double *daa, phydbl *pi);
int Init_Qmat_MtArt(double *daa, double *pi); // Added by Federico Abascal
int Init_Qmat_HIVb(double *daa, double *pi);  // Added by Federico Abascal
int Init_Qmat_HIVw(double *daa, double *pi);  // Added by Federico Abascal
void Switch_From_Mod_To_M4mod(model *mod);
void Switch_From_M4mod_To_Mod(model *mod);

#endif
