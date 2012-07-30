/*

PhyML:  a program that  computes maximum likelihood phyLOGenies from
DNA or AA homoLOGous sequences.

Copyright (C) Stephane Guindon. Oct 2003 onward.

All parts of the source except where indicated are distributed under
the GNU public licence. See http://www.opensource.org for details.

*/

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
#include "mixtlk.h"
#include "mixt.h"

#ifdef MPI
#include "mpi_boot.h"
#endif

#ifdef PHYML

int main(int argc, char **argv)
{
  
  calign *cdata;
  option *io;
  t_tree *tree;
  int n_otu, num_data_set;
  int num_tree,tree_line_number,num_rand_tree;
  matrix *mat;
  t_mod *mod;
  time_t t_beg,t_end;
  phydbl best_lnL,most_likely_size,tree_size;
  int r_seed;
  char *most_likely_tree=NULL;

  
#ifdef MPI
  int rc;
  rc = MPI_Init(&argc,&argv);
  if (rc != MPI_SUCCESS) {
    PhyML_Printf("\n. Error starting MPI program. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }
  MPI_Comm_size(MPI_COMM_WORLD,&Global_numTask);
  MPI_Comm_rank(MPI_COMM_WORLD,&Global_myRank);
#endif

#ifdef QUIET
  setvbuf(stdout,NULL,_IOFBF,2048);
#endif

  tree             = NULL;
  mod              = NULL;
  best_lnL         = UNLIKELY;
  most_likely_size = -1.0;
  tree_size        = -1.0;

  io = (option *)Get_Input(argc,argv);
  r_seed = (io->r_seed < 0)?(time(NULL)):(io->r_seed);
  srand(r_seed);
  io->r_seed = r_seed;

  if(io->in_tree == 2) Test_Multiple_Data_Set_Format(io);
  else io->n_trees = 1;

  if(io->n_trees == 0 && io->in_tree == 2)
    {
      PhyML_Printf("\n. The input tree file does not provide a tree in valid format.");
      Exit("\n");
    }

  mat = NULL;
  tree_line_number = 0;

  if((io->n_data_sets > 1) && (io->n_trees > 1))
    {
      io->n_data_sets = MIN(io->n_trees,io->n_data_sets);
      io->n_trees     = MIN(io->n_trees,io->n_data_sets);
    }

  For(num_data_set,io->n_data_sets)
    {
      n_otu = 0;
      best_lnL = UNLIKELY;
      Get_Seq(io);
      Make_Model_Complete(io->mod);
      Set_Model_Name(io->mod);
      Print_Settings(io);
      mod = io->mod;
        
      if(io->data)
	{
	  if(io->n_data_sets > 1) PhyML_Printf("\n. Data set [#%d]\n",num_data_set+1);
	  cdata = Compact_Data(io->data,io);

	  Free_Seq(io->data,cdata->n_otu);
	  
	  if(cdata) Check_Ambiguities(cdata,io->datatype,io->state_len);
	  else
	    {
	      PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
	      Warn_And_Exit("");
	    }

	  for(num_tree=(io->n_trees == 1)?(0):(num_data_set);num_tree < io->n_trees;num_tree++)
	    {
	      if(!io->mod->s_opt->random_input_tree) io->mod->s_opt->n_rand_starts = 1;

	      For(num_rand_tree,io->mod->s_opt->n_rand_starts)
		{
		  if((io->mod->s_opt->random_input_tree) && (io->mod->s_opt->topo_search != NNI_MOVE))
		    if(!io->quiet) PhyML_Printf("\n. [Random start %3d/%3d]\n",num_rand_tree+1,io->mod->s_opt->n_rand_starts);

		  Init_Model(cdata,mod,io);

		  if(io->mod->use_m4mod) M4_Init_Model(mod->m4mod,cdata,mod);

		  switch(io->in_tree)
		    {
		    case 0 : case 1 : { tree = Dist_And_BioNJ(cdata,mod,io); break; }
		    case 2 :          { tree = Read_User_Tree(cdata,mod,io); break; }
		    }

 		  if(io->fp_in_constraint_tree != NULL) 
		    {
		      char *s;
		      io->cstr_tree = Read_Tree_File_Phylip(io->fp_in_constraint_tree);
		      s = Add_Taxa_To_Constraint_Tree(io->fp_in_constraint_tree,cdata);
		      fflush(NULL);
		      if(tree->mat) Free_Mat(tree->mat);
		      Free_Tree(tree);
		      tree = Read_Tree(&s);
		      io->in_tree = 2;
		      Free(s);
		      Check_Constraint_Tree_Taxa_Names(io->cstr_tree,cdata);
		      Alloc_Bip(io->cstr_tree);  
		      Get_Bip(io->cstr_tree->t_nodes[0],
			      io->cstr_tree->t_nodes[0]->v[0],
			      io->cstr_tree);
		      if(!tree->has_branch_lengths) Add_BioNJ_Branch_Lengths(tree,cdata,mod);
		    }

		  if(!tree) continue;

		  time(&t_beg);
		  time(&(tree->t_beg));
		  
		  tree->mod         = mod;
		  tree->io          = io;
		  tree->data        = cdata;
		  tree->n_pattern   = tree->data->crunch_len;
                  
                  Set_Both_Sides(YES,tree);     

		  if(mod->s_opt->random_input_tree) Random_Tree(tree);

		  if((!num_data_set) && (!num_tree) && (!num_rand_tree)) Check_Memory_Amount(tree);

		  if(io->cstr_tree && !Check_Topo_Constraints(tree,io->cstr_tree))
		    {
		      PhyML_Printf("\n\n. The initial tree does not satisfy the topological constraint.");
		      PhyML_Printf("\n. Please use the user input tree option with an adequate tree topology.");
		      Exit("\n");
		    }

		  Prepare_Tree_For_Lk(tree);
		  Br_Len_Not_Involving_Invar(tree);
		  Unscale_Br_Len_Multiplier_Tree(tree);

		  
		  /* ///////////////////////////////////////// */
		  /* Make_Mixtmod(3,tree); */
		  



		  if(io->in_tree == 1) Spr_Pars(tree);
		 
		  if(io->do_alias_subpatt)
		    {
		      tree->update_alias_subpatt = YES;
		      Lk(tree);
		      tree->update_alias_subpatt = NO;
		    }		  

		  if(tree->mod->s_opt->opt_topo)
		    {
		      if(tree->mod->s_opt->topo_search      == NNI_MOVE) Simu_Loop(tree);
		      else if(tree->mod->s_opt->topo_search == SPR_MOVE) Speed_Spr_Loop(tree);
		      else                                               Best_Of_NNI_And_SPR(tree);

		      if(tree->n_root) Add_Root(tree->t_edges[0],tree);
		    }
		  else
		    {
		      if(tree->mod->s_opt->opt_subst_param || 
			 tree->mod->s_opt->opt_bl)                       Round_Optimize(tree,tree->data,ROUND_MAX);
		      else                                               Lk(tree);
		    }


		  tree->both_sides = 1;
		  Lk(tree);
		  Pars(tree);
		  Get_Tree_Size(tree);
		  PhyML_Printf("\n\n. Log likelihood of the current tree: %f.\n",tree->c_lnL);

		  Br_Len_Involving_Invar(tree);
		  Rescale_Br_Len_Multiplier_Tree(tree);

		  if(!tree->n_root) Get_Best_Root_Position(tree);

		  /* Print the tree estimated using the current random (or BioNJ) starting tree */
		  if(io->mod->s_opt->n_rand_starts > 1)
		    {
		      Print_Tree(io->fp_out_trees,tree);
		      fflush(NULL);
		    }

		  /* Record the most likely tree in a string of characters */
		  if(tree->c_lnL > best_lnL)
		    {
		      best_lnL = tree->c_lnL;
		      if(most_likely_tree) Free(most_likely_tree);
		      most_likely_tree = Write_Tree(tree,NO);
		      most_likely_size = Get_Tree_Size(tree);
		    }

/* 		  JF(tree); */

		  time(&t_end);

		  Print_Fp_Out(io->fp_out_stats,t_beg,t_end,tree,
			       io,num_data_set+1,
			       (tree->mod->s_opt->n_rand_starts > 1)?
			       (num_rand_tree):(num_tree));
		  
		  if(tree->io->print_site_lnl) Print_Site_Lk(tree,io->fp_out_lk);

		  /* Start from BioNJ tree */
		  if((num_rand_tree == io->mod->s_opt->n_rand_starts-1) && (tree->mod->s_opt->random_input_tree))
		    {
		      /* Do one more iteration in the loop, but don't randomize the tree */
		      num_rand_tree--;
		      tree->mod->s_opt->random_input_tree = 0;
		    }
		  
 		  if(io->fp_in_constraint_tree != NULL) Free_Tree(io->cstr_tree);
		  Free_Spr_List(tree);
		  Free_One_Spr(tree->best_spr);
		  if(tree->mat) Free_Mat(tree->mat);
		  Free_Triplet(tree->triplet_struct);
		  Free_Tree_Pars(tree);
		  Free_Tree_Lk(tree);
		  Free_Tree(tree);
		}
	      

	      /* Launch bootstrap analysis */
	      if(mod->bootstrap) 
		{
		  if(!io->quiet) PhyML_Printf("\n. Launch bootstrap analysis on the most likely tree...\n");

                  #ifdef MPI
		  MPI_Bcast (most_likely_tree, strlen(most_likely_tree)+1, MPI_CHAR, 0, MPI_COMM_WORLD);
		  if(!io->quiet)  PhyML_Printf("\n. The bootstrap analysis will use %d CPUs.\n",Global_numTask);
		  #endif

		  most_likely_tree = Bootstrap_From_String(most_likely_tree,cdata,mod,io);
		}
	      else if(io->ratio_test) 
		{
		  /* Launch aLRT */
		  if(!io->quiet) PhyML_Printf("\n. Compute fast branch supports on the most likely tree...\n");
		  most_likely_tree = aLRT_From_String(most_likely_tree,cdata,mod,io);
		}

	      /* Print the most likely tree in the output file */
	      if(!io->quiet) PhyML_Printf("\n. Printing the most likely tree in file '%s'...\n", Basename(io->out_tree_file));
	      if(io->n_data_sets == 1) rewind(io->fp_out_tree);
	      PhyML_Fprintf(io->fp_out_tree,"%s\n",most_likely_tree);
	      

	      if(io->n_trees > 1 && io->n_data_sets > 1) break;
	    }
	  Free_Cseq(cdata);
	}
      else
	{
	  PhyML_Printf("\n. No data was found.\n");
	  PhyML_Printf("\n. Err in file %s at line %d\n",__FILE__,__LINE__);
	  Warn_And_Exit("");
	}
      Free_Model_Complete(mod);
    }
  
  if(most_likely_tree) Free(most_likely_tree);

  if(mod->s_opt->n_rand_starts > 1) PhyML_Printf("\n. Best log likelihood: %f\n",best_lnL);

  Free_Optimiz(mod->s_opt);
  Free_Custom_Model(mod);
  Free_Model_Basic(mod);
  M4_Free_M4_Model(mod->m4mod);
  Free(mod);

  if(io->fp_in_constraint_tree) fclose(io->fp_in_constraint_tree);
  if(io->fp_in_align)           fclose(io->fp_in_align);
  if(io->fp_in_tree)            fclose(io->fp_in_tree);
  if(io->fp_out_lk)             fclose(io->fp_out_lk);
  if(io->fp_out_tree)           fclose(io->fp_out_tree);
  if(io->fp_out_trees)          fclose(io->fp_out_trees);
  if(io->fp_out_stats)          fclose(io->fp_out_stats);

  Free_Input(io);

  time(&t_end);
  Print_Time_Info(t_beg,t_end);

#ifdef MPI
  MPI_Finalize();
#endif

  return 0;
}


#elif(M4)
#include "m4.h"
int main(int argc, char **argv)
{
  M4_main(argc, argv);
  return 1;
}

#elif(PART)
#include "mg.h"
int main(int argc, char **argv)
{
  PART_main(argc, argv);
  return 1;
}

#elif(PHYTIME)
#include "times.h"
int main(int argc, char **argv)
{
  TIMES_main(argc, argv);
  return 1;
}

#elif(PHYCONT)
#include "continuous.h"
int main(int argc, char **argv)
{
  CONT_main(argc, argv);
  return 1;
}

#elif(RF)
int main(int argc, char **argv)
{
  t_tree *tree1, *tree2;
  FILE *fp_tree1, *fp_tree2;
  int i,j;

  fp_tree1 = (FILE *)fopen(argv[1],"r");
  fp_tree2 = (FILE *)fopen(argv[2],"r");

  tree1 = Read_Tree_File_Phylip(fp_tree1);
  tree2 = Read_Tree_File_Phylip(fp_tree2);

  Match_Nodes_In_Small_Tree(tree1,tree2);

  For(i,2*tree1->n_otu-2)
    {
      printf("\n. Node %d in tree1 matches node %d in tree2",i,(tree1->noeud[i]->match_node)?(tree1->noeud[i]->match_node->num):(-1));
    }




/*   t_tree *tree1, *tree2; */
/*   FILE *fp_tree1, *fp_tree2; */
/*   int i,j,rf,n_edges,n_common,bip_size; */
/*   phydbl thresh; */
/*   t_edge *b; */


/*   fp_tree1 = (FILE *)fopen(argv[1],"r"); */
/*   fp_tree2 = (FILE *)fopen(argv[2],"r"); */
/*   thresh = (phydbl)atof(argv[3]); */

/*   tree1 = Read_Tree_File(fp_tree1); */
/*   tree2 = Read_Tree_File(fp_tree2); */

/*   Get_Rid_Of_Prefix('_',tree1); */

/* /\*   Find_Common_Tips(tree1,tree2); *\/ */

/*   Alloc_Bip(tree1); */
/*   Alloc_Bip(tree2); */

/*   Get_Bip(tree1->noeud[0],tree1->noeud[0]->v[0],tree1); */
/*   Get_Bip(tree2->noeud[0],tree2->noeud[0]->v[0],tree2); */
  
/* /\*   PhyML_Printf("\n. rf=%f\n",Compare_Bip_On_Existing_Edges(thresh,tree1,tree2)); *\/ */
/*   For(i,2*tree1->n_otu-3) tree1->t_edges[i]->bip_score = 0; */
/*   For(i,2*tree2->n_otu-3) tree2->t_edges[i]->bip_score = 0; */
  
/*   rf = 0; */
/*   n_edges = 0; */

/*   /\* First tree *\/ */
/*   For(i,2*tree1->n_otu-3)  */
/*     { */
/*       /\* Consider the branch only if the corresponding bipartition has size > 1 *\/ */
/*       b = tree1->t_edges[i]; */
/*       bip_size = MIN(b->left->bip_size[b->l_r],b->rght->bip_size[b->r_l]); */
	  
/*       if(bip_size > 1) */
/* 	{ */
/* 	  /\* with non-zero length *\/ */
/* 	  if(tree1->t_edges[i]->l > thresh)   */
/* 	    { */
/* 	      n_edges++; */
/* 	      /\* This t_edge is not found in tree2 *\/ */
/* 	      if(!tree1->t_edges[i]->bip_score) rf++; ; */
/* 	    } */
/* 	} */
/*     } */


/*   /\* Second tree *\/ */
/*   For(i,2*tree2->n_otu-3)  */
/*     { */
/*       b = tree2->t_edges[i]; */
/*       bip_size = MIN(b->left->bip_size[b->l_r],b->rght->bip_size[b->r_l]); */

/*       if(bip_size > 1) */
/* 	{ */
/* 	  if(tree2->t_edges[i]->l > thresh)   */
/* 	    { */
/* 	      n_edges++; */
/* 	      /\* This t_edge is not found in tree1 *\/ */
/* 	      if(!tree2->t_edges[i]->bip_score) rf++; ; */
/* 	    } */
/* 	} */
/*     } */

/*   if(!n_edges) */
/*     { */
/*       Exit("\n. No comparable internal edges were found.\n"); */
/*     } */
/*   else */
/*     { */
/*       PhyML_Printf("\n. Robinson and Foulds distance: %f.",(double)rf/(n_edges)); */
/* /\*       PhyML_Printf("\n. %d internal edges were processed (%d in the first tree, %d in the second).\n",n_edges,n_edges_t1,n_edges-n_edges_t1); *\/ */
/*       PhyML_Printf("\n"); */
/*     } */

  return 1;
}

#elif(TIPORDER)
#include "tiporder.h"
int main(int argc, char **argv)
{
  TIPO_main(argc, argv);
  return 1;
}


#elif(TEST)
#include "xml.h"
int main(int argc, char **argv)
{
  FILE *fp;
  xml_node *root,*p_elem,*m_elem,*d_elem,*parent,*instance;

  fp = fopen(argv[1],"r");
  
  root = XML_Load_File(fp);  
  

  option *io;
  void *buff;
  t_mod *mod,*iomod;
  t_tree *tree,*mixt_tree,*root_tree;
  char *alignment;
  int select;
  char *component;
  int i,j,n_components;
  int first_m_elem;
  int class_number;
  scalar_dbl **lens;

  component = (char *)mCalloc(T_MAX_NAME,sizeof(char));

  d_elem       = NULL;
  m_elem       = NULL;
  p_elem       = root;
  io           = NULL;
  mixt_tree    = NULL;
  root_tree    = NULL;
  mod          = NULL;
  tree         = NULL;
  lens         = NULL;
  select       = -1;
  class_number = -1;
  // Make sure there are no duplicates in node's IDs
  XML_Check_Duplicate_ID(root);


  int count = 0;
  XML_Count_Number_Of_Node_With_Name("topology",&count,root);
  
  if(count > 1)
    {
      PhyML_Printf("\n== There should not more than one 'topology' node.");
      PhyML_Printf("\n== Found %d. Please fix your XML file",count);
      Exit("\n");
    }
  else if(count < 1)
    {
      PhyML_Printf("\n== There should be at least one 'topology' node.");
      PhyML_Printf("\n== Found none. Please fix your XML file");
      Exit("\n");
    }
  


  // Read all partitionelem nodes and mixturelem nodes in each of them
  do
    {
      p_elem = XML_Search_Node_Name("partitionelem",YES,p_elem);
      if(p_elem == NULL) break;
      
     
      buff = (option *)Make_Input();
      Set_Defaults_Input(buff);
      if(io) 
	{
	  io->next = buff;
	  io->next->prev = io;
	  io = io->next;
	}
      else io = buff;

      // Set the datatype (required when compressing data)
      char *dt = NULL;
      dt = XML_Get_Attribute_Value(p_elem,"datatype");
      if(!dt)
	{
	  PhyML_Printf("\n== Please specify the type of data ('aa' or 'nt') for partition element '%s'",
		       XML_Get_Attribute_Value(p_elem,"id"));
	  Exit("\n");	    
	}

      select = XML_Validate_Attr_Int(dt,2,"aa","nt");
      switch(select)
	{
	case 0: 
	  {
	    io->datatype = AA;
	    break;
	  }
	case 1:
	  {
	    io->datatype = NT;
	    break;
	  }
	default:
	  {
	    PhyML_Printf("\n== Unknown data type. Must be either 'aa' or 'nt'.");
	    Exit("\n");
	  }
	}

      // Attach a model to this io struct
      io->mod = (t_mod *)Make_Model_Basic();
      Set_Defaults_Model(io->mod);
      io->mod->ras->n_catg = 1;
      io->mod->io = io;

      iomod = io->mod;

      // Attach an optimization structure to this model
      iomod->s_opt = (t_opt *)Make_Optimiz();
      Set_Defaults_Optimiz(iomod->s_opt);

      iomod->s_opt->opt_kappa  = NO;
      iomod->s_opt->opt_lambda = NO;
      iomod->s_opt->opt_rr     = NO;

      // Input file
      alignment = XML_Get_Attribute_Value(p_elem,"filename");  
      
      strcpy(io->in_align_file,alignment);
      io->fp_in_align = Openfile(io->in_align_file,0);
      strcpy(io->out_tree_file,alignment);
      strcat(io->out_tree_file,"_phyml_tree");
      strcpy(io->out_stats_file,alignment);
      strcat(io->out_stats_file,"_phyml_stats");      

      // Load sequence file
      io->data  = Get_Seq(io);

      // Compress alignment
      io->cdata = Compact_Data(io->data,io);

      // Create new mixture tree
      buff = (t_tree *)Make_Tree_From_Scratch(io->cdata->n_otu,
					      io->cdata);

      if(mixt_tree)
	{
	  mixt_tree->next = buff;
	  mixt_tree->next->prev = mixt_tree;
	  mixt_tree = mixt_tree->next;
	}
      else mixt_tree = buff;
      
      // mixt_tree is a mixture tree
      mixt_tree->is_mixt_tree = YES;

      // Connect mixt_tree to io struct
      mixt_tree->io = io;

      // Connect mixt_tree to model struct
      mixt_tree->mod = iomod;

      // Connect mixt_tree to compressed data
      mixt_tree->data = io->cdata;

      // Set total number of patterns
      mixt_tree->n_pattern = io->cdata->crunch_len;


      // Connect last tree of the mixture for the
      // previous partition element to the next mixture tree
      if(tree) tree->next = mixt_tree;

      // Do the same for the model
      if(mod)  mod->next = iomod;

      if(!root_tree) root_tree = mixt_tree;

      printf("\n. read partitionelem %s",XML_Get_Attribute_Value(p_elem,"id"));

      // Process all the mixtureelem tags in this partition element
      n_components  = 0;
      m_elem        = p_elem;
      first_m_elem  = 0;
      mod           = NULL;
      tree          = NULL;
      class_number  = 0;
      do
	{
	  m_elem = XML_Search_Node_Name("mixtureelem",YES,m_elem);
	  if(m_elem == NULL) break;
	  
	  if(!strcmp(m_elem->name,"mixtureelem"))
	    {
	      first_m_elem++;
	      
	      // Rewind tree and model when processing a new mixtureelem node
	      if(first_m_elem > 1) 
		{
		  while(tree->prev) { tree = tree->prev; } // tree = tree->parent->child;
		  while(mod->prev)  { mod  = mod->prev;  } // mod = mod->parent->child;
		}

	      // Read and process model components
	      char *list;
	      list = XML_Get_Attribute_Value(m_elem,"list");

	      j = 0;
	      For(i,(int)strlen(list)) if(list[i] == ',') j++;
	      
	      if(j != n_components && first_m_elem > 1)
		{
		  PhyML_Printf("\n== Discrepancy in the number of elements in nodes 'mixtureelem' partitionelem id '%s'",p_elem->id);
		  PhyML_Printf("\n== Check 'mixturelem' node with list '%s'",list);
		  Exit("\n");
		}
	      n_components = j;

	      i = j = 0;
	      component[0] = '\0';	      
	      while(1)
	      {
		if(list[j] == ',' || j == (int)strlen(list))
		  {
		    // Reading a new component

		    if(first_m_elem == YES) // Only true when processing the first mixtureelem node
		      {
			t_tree *this_tree;
			t_mod *this_mod;

			// Create new tree
			this_tree = (t_tree *)Make_Tree_From_Scratch(io->cdata->n_otu,
								     io->cdata);
			
			if(tree)
			  {
			    tree->next = this_tree;
			    tree->next->prev = tree;
			  }
			else 
			  {
			    mixt_tree->child = this_tree;
			  }

			tree         = this_tree;			
			tree->parent = mixt_tree;
			
			// Create a new model
			this_mod = (t_mod *)Make_Model_Basic();
			Set_Defaults_Model(this_mod);
			this_mod->ras->n_catg = 1;

			if(mod)
			  {
			    mod->next = this_mod;
			    mod->next->prev = mod;
			  }
			
			mod = this_mod;
			mod->parent = iomod;
			if(!iomod->child) iomod->child = mod;
			mod->io = io;

			mod->s_opt = (t_opt *)Make_Optimiz();
			Set_Defaults_Optimiz(mod->s_opt);
			
			mod->s_opt->opt_alpha  = NO;
			mod->s_opt->opt_pinvar = NO;
			
			tree->data      = io->cdata;
			tree->n_pattern = io->cdata->crunch_len;
			tree->io        = io;
			tree->mod       = mod;


			if(tree->n_pattern != tree->parent->n_pattern)
			  {
			    PhyML_Printf("\n== Err in file %s at line %d\n",__FILE__,__LINE__);
			    Warn_And_Exit("");
			  }
		      }

		    // Read a component
		    component[i] = '\0';		    
		    if(j != (int)strlen(list)-1) i = 0;
		    
		    // Find which node this ID corresponds to
		    instance = XML_Search_Node_ID(component,YES,root);
		  
		    if(!instance)
		      {
			PhyML_Printf("\n== Could not find a node with id:'%s'.",component);
			PhyML_Printf("\n== Problem with 'mixtureelem' node, list '%s'.",list);
			Exit("\n");
		      }
		    if(!instance->parent)
		      {
			PhyML_Printf("\n== Node '%s' with id:'%s' has no parent.",instance->name,component);
			Exit("\n");
		      }
		      
		    parent = instance->parent;


		    ////////////////////////////////////////
		    //        SUBSTITUTION MODEL          //
		    ////////////////////////////////////////

		    if(!strcmp(parent->name,"ratematrices"))
		      {
			// Init substitution model here
			char *model = NULL;

			model = XML_Get_Attribute_Value(instance,"model");  
			
			if(model == NULL)
			  {
			    PhyML_Printf("\n== Poorly formated XML file.");
			    PhyML_Printf("\n== Attribute 'model' is mandatory in a <ratematrix> node.");
			    Exit("\n");
			  }

			select = XML_Validate_Attr_Int(model,26,
						       "xxxxx",    //0
						       "JC69",     //1
						       "K80",      //2
						       "F81",      //3
						       "HKY85",    //4
						       "F84",      //5
						       "TN93",     //6
						       "GTR",      //7
						       "CUSTOM",   //8
						       "xxxxx",    //9
						       "xxxxx",    //10
						       "WAG",      //11
						       "DAYHOFF",  //12
						       "JTT",      //13
						       "BLOSUM62", //14
						       "MTREV",    //15
						       "RTREV",    //16
						       "CPREV",    //17
						       "DCMUT",    //18
						       "VT",       //19
						       "MTMAM",    //20
						       "MTART",    //21
						       "HIVW",     //22
						       "HIVB",     //23
						       "CUSTOMAA", //24
						       "LG");      //25

			if(select < 9)
			  {
			    mod->ns = 4;
			    if(io->datatype != NT)
			      {
				PhyML_Printf("\n== Data type and selected model are incompatible");
				Exit("\n");
			      }
			  }
			else
			  {
			    mod->ns = 20;
			    if(io->datatype != AA)
			      {
				PhyML_Printf("\n== Data type and selected model are incompatible");
				Exit("\n");
			      }
			  }
			
			io->mod->ns = mod->ns;

			// Set model number & name
			mod->whichmodel = Set_Whichmodel(select);	
			Set_Model_Name(mod);

			if(mod->whichmodel == K80 || mod->whichmodel == HKY85 || mod->whichmodel == TN93)
			  {
			    char *tstv;
			    tstv = XML_Get_Attribute_Value(instance,"tstv");
			    
			    if(tstv)
			      {
				if(!strcmp(tstv,"estimate") || !strcmp(tstv,"estimated") || 
				   !strcmp(tstv,"optimize") || !strcmp(tstv,"optimized"))
				  {
				    mod->s_opt->opt_kappa = YES;
				  }
				else
				  {					  
				    mod->s_opt->opt_kappa = NO;
				    mod->kappa->v = String_To_Dbl(tstv);
				  }
			      }
			    else
			      {
				mod->s_opt->opt_kappa = YES;
			      }
			  }
			else
			  {
			    mod->s_opt->opt_kappa = NO;
			  }


			if(mod->whichmodel == GTR || mod->whichmodel == CUSTOM)
			  {
			    char *rr;
			    rr = XML_Get_Attribute_Value(instance,"rr");
			    
			    if(rr)
			      {
				if(!strcmp(rr,"estimate") || !strcmp(rr,"estimated") || 
				   !strcmp(rr,"optimize") || !strcmp(rr,"optimized"))
				  {
				    mod->s_opt->opt_rr = YES;
				  }
			      }
			  }

			/*! Custom model for amino-acids. Read in the rate matrix file */
			if(mod->whichmodel == CUSTOMAA)
			  {
			    char *r_mat_file;

			    r_mat_file = XML_Get_Attribute_Value(instance,"ratematrixfile");  

			    if(!r_mat_file)
			      {
				PhyML_Printf("\n== No valid 'ratematrixfile' attribute could be processed.\n");
				PhyML_Printf("\n== Please fix your XML file.\n");
				Exit("\n");
			      }
			    else
			      {
				io->fp_aa_rate_mat = Openfile(r_mat_file,0);
				strcpy(io->aa_rate_mat_file,r_mat_file);
			      }
			  }

			// If n->ds == NULL, the corrresponding node data structure, n->ds, has not
			// been initialized. If not, do nothing.
			if(instance->ds->obj == NULL)  
			  {
			    instance->ds->obj       = (t_rmat *)Make_Rmat(mod->ns);			    

			    instance->ds->next      = (t_ds *)mCalloc(1,sizeof(t_ds));

			    Free(mod->kappa);
			    instance->ds->next->obj = (scalar_dbl *)mCalloc(1,sizeof(scalar_dbl));
			    
			  }

			// Connect the data structure n->ds to mod->r_mat
			mod->r_mat = (t_rmat *)instance->ds->obj;
			mod->kappa = (scalar_dbl *)instance->ds->next->obj;
		      }

		    ////////////////////////////////////////
		    //           STATE FREQS              //
		    ////////////////////////////////////////

		    else if(!strcmp(instance->name,"statefreqs"))
		      {
			// Stationary frequencies

			// If n->ds == NULL, the corrresponding node data structure, n->ds, has not
			// been initialized. If not, do nothing.
			if(instance->ds->obj == NULL)  instance->ds->obj = (t_efrq *)Make_Efrq(mod->ns);

			// Connect the data structure n->ds to mod->e_frq
			mod->e_frq = (t_efrq *)instance->ds->obj;		      	      
		      }


		    //////////////////////////////////////////
		    //             TOPOLOGY                 //
		    //////////////////////////////////////////

		    else if(!strcmp(parent->name,"topology"))
		      {
			// Starting tree
			
			char *starting_tree = NULL;
			starting_tree = XML_Get_Attribute_Value(instance,"value");
			    
			if(starting_tree)
			  {
			    // Starting tree
			    if(strcmp(starting_tree,"bionj"))
			      {
				if(!Filexists(starting_tree))
				  {
				    PhyML_Printf("\n== The tree file '%s' could not be found.",starting_tree);
				    Exit("\n");
				  }
				else
				  {
				    strcpy(io->in_tree_file,starting_tree);
				    io->in_tree = 2;
				    io->fp_in_tree = Openfile(io->in_tree_file,0);
				  }
			      }
			  }

			// Estimate tree topology
			char *estimate = NULL;
			
			estimate = XML_Get_Attribute_Value(instance,"estimate");
			
			if(estimate)
			  {
			    int select;
			    
			    select = XML_Validate_Attr_Int(estimate,6,
							   "true","yes","y",
							   "false","no","n");
			    
			    if(select > 2) io->mod->s_opt->opt_topo = NO;
			    else
			      {
				char *search;
				int select;
				
				search = XML_Get_Attribute_Value(instance,"search");				    
				select = XML_Validate_Attr_Int(search,3,"spr","nni","best");
				
				switch(select)
				  {
				  case 0:
				    {
				      io->mod->s_opt->topo_search = SPR_MOVE;
				      break;
				    }
				  case 1:
				    {
				      io->mod->s_opt->topo_search = NNI_MOVE;
				      break;
				    }
				  case 2:
				    {
				      io->mod->s_opt->topo_search = BEST_OF_NNI_AND_SPR;
				      break;
				    }
				  default:
				    {
				      PhyML_Printf("\n== Topology search option '%s' is not valid.",search);
				      Exit("\n");
				      break;
				    }
				  }
			      }
			  }
		      }

		    //////////////////////////////////////////
		    //                RAS                   //
		    //////////////////////////////////////////

		    else if(!strcmp(parent->name,"siterates"))
		      {
			char *rate_value = NULL;			
			scalar_dbl *r;
			

			// First time we process a 'siterates' node, check that its format is valid.
			// and process it afterwards.
			if(parent->ds->obj == NULL)
			  {
			    xml_node *w;
			    char *family;
			    int select;
			    
			    iomod->ras->n_catg = 0;
			    class_number = 0;

			    XML_Check_Siterates_Node(parent);
			    
			    w = XML_Search_Node_Name("weights",YES,parent);
			    if(w)
			      {
				family = XML_Get_Attribute_Value(w,"family");
				select = XML_Validate_Attr_Int(family,3,"gamma","gamma+inv","freerates");
				switch(select)
				  {
				  case 0: // Gamma model
				    {
				      char *alpha;

				      iomod->s_opt->opt_pinvar = NO;
				      iomod->ras->invar        = NO;

				      alpha = XML_Get_Attribute_Value(w,"alpha");
				      
				      if(alpha)
					{
					  if(!strcmp(alpha,"estimate") || !strcmp(alpha,"estimated") || 
					     !strcmp(alpha,"optimize") || !strcmp(alpha,"optimized"))
					    {
					      iomod->s_opt->opt_alpha = YES;
					    }
					  else
					    {					  
					      iomod->s_opt->opt_alpha = NO;
					      iomod->ras->alpha->v = String_To_Dbl(alpha);
					    }
					}
				      
				      iomod->ras->n_catg = XML_Siterates_Number_Of_Classes(parent);
				      
				      Make_RAS_Complete(iomod->ras);

				      break;
				    }
				  case 1: // Gamma+Inv model
				    {
				      char *alpha;

				      iomod->ras->invar        = YES;
				      iomod->s_opt->opt_pinvar = YES;

				      alpha = XML_Get_Attribute_Value(w,"alpha");
				      
				      if(alpha)
					{
					  if(!strcmp(alpha,"estimate") || !strcmp(alpha,"estimated") || 
					     !strcmp(alpha,"optimize") || !strcmp(alpha,"optimized"))
					    {
					      iomod->s_opt->opt_alpha = YES;
					    }
					  else
					    {
					      iomod->s_opt->opt_alpha = NO;
					      iomod->ras->alpha->v = String_To_Dbl(alpha);;
					    }
					}
				      
				      iomod->ras->n_catg = XML_Siterates_Number_Of_Classes(parent);
				      iomod->ras->n_catg--;

				      break;
				    }
				  case 2: // FreeRate model
				    {
				      char *est_weights;
				      int select;

				      iomod->ras->free_mixt_rates = YES;
				      iomod->s_opt->opt_free_mixt_rates = YES;				      
				      iomod->ras->n_catg = XML_Siterates_Number_Of_Classes(parent);
				      break;
				    }
				  default:
				    {
				      PhyML_Printf("\n== family: %s",family);
				      PhyML_Printf("\n== Err in file %s at line %d\n",__FILE__,__LINE__);
				      Exit("\n");
				    }
				  }
			      }
			   
			    int nc = XML_Get_Number_Of_Classes_Siterates(parent);

			    if(w && nc != iomod->ras->n_catg)
			      {
				PhyML_Printf("\n== 'Siterates' node. The number of 'weight' classes");
				PhyML_Printf("\n== should be the same as that of 'instance' nodes.");
				PhyML_Printf("\n== Please fix your XML file accordingly.");
				Exit("\n");
			      }

			    if(!w) iomod->ras->n_catg = nc;

			    PhyML_Printf("\n. Making RAS model with %d classes of rates.\n",iomod->ras->n_catg);

			    Make_RAS_Complete(iomod->ras);

			    t_ds *ds = parent->ds;

			    ds->obj = (t_ras *)iomod->ras;

			    ds->next      = (t_ds *)mCalloc(1,sizeof(t_ds));
			    ds->next->obj = (int *)(&iomod->s_opt->opt_alpha);

			    ds->next->next      = (t_ds *)mCalloc(1,sizeof(t_ds));
			    ds->next->next->obj = (int *)(&iomod->s_opt->opt_free_mixt_rates);
			  }
			else
			  {
			    if(iomod->ras != (t_ras *)parent->ds->obj) Free_RAS(iomod->ras);
			    iomod->ras = (t_ras *)parent->ds->obj;
			    iomod->s_opt->opt_alpha = *((int *)parent->ds->next->obj);
			    iomod->s_opt->opt_free_mixt_rates = *((int *)parent->ds->next->next->obj);
			  }
			
			rate_value = XML_Get_Attribute_Value(instance,"value");

			if(instance->ds->obj == NULL) 
			  {
			    r = (scalar_dbl *)mCalloc(1,sizeof(scalar_dbl));
			    Init_Scalar_Dbl(r);
			    r->v = String_To_Dbl(rate_value);
			    instance->ds->obj = (scalar_dbl *)r;
			  }
			else
			  {
			    r = (scalar_dbl *)instance->ds->obj;
			  }
		       
			// Invariant
			if(Are_Equal(r->v,0.0,1E-20)) 
			  {
			    mod->ras->invar = YES;
			  }
			else
			  {
			    iomod->ras->gamma_rr->v[class_number] = r->v;
			    mod->ras->parent_class_number = class_number;
			    class_number++;
			  }

			xml_node *orig_w = NULL;
    			orig_w = XML_Search_Node_Attribute_Value("appliesto",instance->id,YES,instance->parent);
			
    			if(orig_w)
    			  {
    			    char *weight;
    			    weight = XML_Get_Attribute_Value(orig_w,"value");
			    if(mod->ras->invar == YES)
			      {
				iomod->pinvar->v = String_To_Dbl(weight);
			      }
			    else
			      {
				iomod->ras->gamma_r_proba->v[class_number] = String_To_Dbl(weight);
			      }
    			  }

			/* printf("\n. r->v: %f class_number: %d",r->v,class_number); */
			/* printf("\n. mod->invar: %d parent_class: %d r = %f w = %f", */
			/*        mod->ras->invar, */
			/*        mod->ras->parent_class_number, */
			/*        r->v, */
			/*        iomod->ras->gamma_r_proba->v[class_number]); */

		      }

		    //////////////////////////////////////////////
		    //           BRANCH LENGTHS                 //
		    //////////////////////////////////////////////

		    else if(!strcmp(parent->name,"branchlengths"))
		      {
			int i;
			int n_otu;

			if(instance->ds->obj == NULL)
			  {
			    n_otu = tree->n_otu;
			    lens = (scalar_dbl **)mCalloc(2*tree->n_otu-3,sizeof(scalar_dbl *));
			    For(i,2*tree->n_otu-3)
			      {
			    	lens[i] = (scalar_dbl *)mCalloc(1,sizeof(scalar_dbl));
			    	Init_Scalar_Dbl(lens[i]);
			      }
			    			    
			    instance->ds->obj = (scalar_dbl **)lens;
			    Free(tree->t_edges[i]->l);
			  }
			else
			  {
			    lens = (scalar_dbl **)instance->ds->obj;
			  }
			
			if(n_otu != tree->n_otu)
			  {
			    PhyML_Printf("\n== All the data sets should display the same number of sequences.");
			    PhyML_Printf("\n== Found at least one data set with %d sequences and one with %d sequences.",n_otu,tree->n_otu);
			    Exit("\n");
			  }

			For(i,2*tree->n_otu-3) tree->t_edges[i]->l = lens[i];
		      }

		    //////////////////////////////////////////////
		    //           EQUILIBRIUM FREQS              //
		    //////////////////////////////////////////////

		    else if(!strcmp(parent->name,"statefreqs"))
		      {
		      }

		    else
		      {
			PhyML_Printf("\n== Problem with your XML file format.");
			PhyML_Printf("\n== Encountered when reading component '%s'.",component);
			Exit("\n");
		      }

		    ///////////////////////////////////////////////
		    ///////////////////////////////////////////////
		    ///////////////////////////////////////////////

		    if(first_m_elem > 1) // Done with this component, move to the next tree and model
		      {
			if(tree->next) tree = tree->next;
			if(mod->next)  mod  = mod->next;
		      }

		  }
	      else if(list[j] != ' ')
		  {
		    component[i] = list[j];
		    i++;
		  }
		j++;
		if(j == (int)strlen(list)+1) break;
	      
	      } // end of mixtureelem processing
	      	      
	    } // end of partitionelem processing	  
	}
      while(1);
    }
  while(1);

  while(io->prev != NULL) io = io->prev;
  while(mixt_tree->prev != NULL) mixt_tree = mixt_tree->prev;

  Print_Data_Structure(mixt_tree);
    
  /*! Set default branch lengths if unspecified in the xml file. */
  xml_node *bl = NULL;
  bl = XML_Search_Node_Name("branchlengths",YES,root);
  if(!bl)
    {
      lens = (scalar_dbl **)mCalloc(2*mixt_tree->n_otu-3,sizeof(scalar_dbl *));
      For(i,2*mixt_tree->n_otu-3)
	{
	  lens[i] = (scalar_dbl *)mCalloc(1,sizeof(scalar_dbl));
	  Init_Scalar_Dbl(lens[i]);
	}            

      tree = mixt_tree;
      do
	{
	  For(i,2*tree->n_otu-3) Free(tree->t_edges[i]->l);
	  if(tree->child) tree = tree->child;
	  else            tree = tree->next;
	}while(tree);

      tree = mixt_tree;
      do
	{
	  For(i,2*tree->n_otu-3) tree->t_edges[i]->l = lens[i];
	  if(tree->child) tree = tree->child;
	  else            tree = tree->next;
	}while(tree);      
    }

  option *this_io = io;
  t_tree *this_mixt_tree = mixt_tree;
  do
    {
      if(this_io->in_tree == 2) // user-defined input tree
	{
	  if(!this_io->fp_in_tree)
	    {
	      PhyML_Printf("\n== Err. in file %s at line %d\n",__FILE__,__LINE__);
	      Exit("\n");
	    }
	  
	  // Copy the user tree to all the tree structures
	  t_tree *this_tree;

	  this_tree = Read_User_Tree(this_io->cdata,mod,this_io);
	  Connect_CSeqs_To_Nodes(this_io->cdata,this_tree);	  
	  Copy_Tree(this_tree,this_mixt_tree);

	  this_tree = this_mixt_tree->child;
	  do
	    {
	      Copy_Tree(this_mixt_tree,this_tree);
	      Connect_CSeqs_To_Nodes(io->cdata,this_tree);
	      if(this_tree->is_mixt_tree == YES || 
		 this_tree->next == NULL) break;
	      this_tree = this_tree->next;
	    }
	  while(1);
	}

      this_io        = this_io->next;
      this_mixt_tree = this_mixt_tree->next;

      if(!this_io) break;

    }while(1);

  time(&(mixt_tree->t_beg));

  /*! Finish making the models */
  mod = mixt_tree->mod;
  do
    {
      Make_Model_Complete(mod);      
      if(mod->child) mod = mod->child;
      else           mod = mod->next;
    }
  while(mod);


  /* tree = mixt_tree; */
  /* do */
  /*   { */
  /*     printf("\n. AV RR = %f", */
  /* 	     (tree->parent && tree->mod->ras->invar == NO)?(tree->parent->mod->ras->gamma_rr->v[tree->mod->ras->parent_class_number]):(-1.)); */
  /*     if(tree->child) tree = tree->child; */
  /*     else            tree = tree->next; */
  /*   }while(tree); */


  /*! Initialize the models */
  mod  = mixt_tree->mod;
  do
    {      
      Init_Model(mod->io->cdata,mod,mod->io);
      if(mod->child) mod = mod->child;
      else           mod = mod->next;
    }
  while(mod);


  /*! Connect edges of every tree to next, prev & child
    ! edges in the corresponding trees */
  tree = mixt_tree;
  do
    {
      MIXT_Connect_Edges_To_Next_Prev_Child_Parent(tree);
      MIXT_Connect_Nodes_To_Next_Prev_Child_Parent(tree);
      if(tree->child) tree = tree->child;
      else            tree = tree->next;
    }
  while(tree);


  t_edge *b = NULL;
  For(i,2*mixt_tree->n_otu-3)
    {
      b = mixt_tree->t_edges[i];
      b->l->onoff = ON;
      do
	{
	  if(b->l->onoff == ON)
	    {
	      b->l->v /= (1.-mixt_tree->mod->pinvar->v);
	      b->l->v /= mixt_tree->mod->br_len_multiplier->v;
	      b->l->onoff = OFF;
	    }
	  b = b->next;
	  if(!b) break;
	}while(1);
    }

  /*! Turn all branches to ON state */
  tree = mixt_tree;
  do
    {
      MIXT_Turn_Branches_OnOff(ON,tree);
      if(tree->child) tree = tree->child;
      else            tree = tree->next;
    }
  while(tree);


  /*! Check that each partition element points to a single set of edge lengths */
  tree = mixt_tree;
  do
    {
      if(tree->is_mixt_tree == YES) tree = tree->child;

      if(tree->next) 
	{
	  if((tree->parent == tree->next->parent) && 
	     (tree->t_edges[0]->l != tree->next->t_edges[0]->l))
	    {
	      PhyML_Printf("\n== Only one set of branch lengths is allowed for each data partition.");
	      PhyML_Printf("\n== Please amend your XML file accordingly.");
	      Exit("\n");
	    }
	}

      tree = tree->next;

    }
  while(tree);


  tree = mixt_tree;
  do
    {
      Prepare_Tree_For_Lk(tree);
      /* printf("\n. %p %s",(void *)tree,Write_Tree(tree,NO)); */
      if(tree->child) tree = tree->child;
      else            tree = tree->next;
    }
  while(tree);
  /* Exit("\n"); */


  /*! Connect sprs of every tree to next, prev & child
    ! edges in the corresponding trees */
  tree = mixt_tree;
  do
    {
      MIXT_Connect_Sprs_To_Next_Prev_Child_Parent(tree);
      if(tree->child) tree = tree->child;
      else            tree = tree->next;
    }
  while(tree);

  /* TO DO

    1) REMOVE ROOT
    X 2) ALLOW MORE THAN ONE VECTOR OF BRANCH LENGTHS -> NEED TO 
       LOOK CAREFULY AT WHAT IT MEANS FOR SPR,  SIMULTANEOUS NNI MOVES
       PRUNE AND REGRAFT...
    X 3) Branch lengths in Prune and Regarft -> make sure you don't apply
       change several times
    4) Make sure you can't have the same branch lengths with distinct
       data types
    5) Finish rewritting Opt_Free_Param
    X 6) Make sure you can have only one set of branch lengths per partition
    7) BIONJ starting tree

  */


  PhyML_Printf("\n. Calculating the likelihood now");

  tree = mixt_tree;
  do
    {
      Set_Both_Sides(YES,tree);
      if(tree->child) tree = tree->child;
      else            tree = tree->next;
    }
  while(tree);

  Lk(NULL,mixt_tree);
  PhyML_Printf("\n. lnL=%12f",mixt_tree->c_lnL);

  printf("\n\n\n");

  if(!Check_Lk_At_Given_Edge(mixt_tree)) Exit("\n. FAILED HERE\n");
  
  printf("\n. <<<<<<<<<<< OPTIMIZ >>>>>>>>>>>>>>>>>>\n");


  /* Simu_Loop(mixt_tree); */
  /* Speed_Spr_Loop(mixt_tree); */
  Round_Optimize(mixt_tree,mixt_tree->data,ROUND_MAX);

  PhyML_Printf("\n. FINAL: lnL=%12f",mixt_tree->c_lnL);

  Free(component);

  XML_Free_XML_Tree(root);

  fclose(fp);

  return 1;
}

#endif

