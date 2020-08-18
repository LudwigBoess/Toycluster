extern void Build_Tree();
extern int Find_ngb_tree(const size_t, const double, int*);
extern int *Find_ngb_tree_recursive(size_t, double, int);
int Find_ngb_simple(const int ipart,  const double hsml, int *ngblist);
extern double Guess_hsml(const size_t ipart, const int DesNumNgb);
int Ngbcnt ;
int Ngblist[NGBMAX];
