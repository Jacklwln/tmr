#include "TMRForrest.h"
#include "isoFSDTStiffness.h"
#include "TACSCreator.h"
#include "TACSAssembler.h"
#include "PlaneStressQuad.h"
#include "TACSToFH5.h"
#include "TACSShellTraction.h"
#include "TensorToolbox.h"
#include "MITCShell.h"
#include "KSFailure.h"
#include "tacslapack.h"

// Create the nodal locations for the mesh
/*
const double test_Xpts[] = {0.0, 0.0, 0.0,
                            1.0, 0.0, 0.0,
                            0.3, 0.7, 0.0,
                            0.8, 0.2, 0.0,
                            0.25, 0.21, 0.0,
                            0.75, 0.6, 0.0,
                            0.0, 1.0, 0.0,
                            1.0, 1.0, 0.0};
*/
const double test_Xpts[] = {0.0, 0.0, 0.0,
                            1.0, 0.0, 0.0,
                            0.3, 0.7, 0.0,
                            0.7, 0.3, 0.0,
                            0.3, 0.3, 0.0,
                            0.7, 0.7, 0.0,
                            0.0, 1.0, 0.0,
                            1.0, 1.0, 0.0};

const int test_conn[] = {0, 1, 4, 3,
                         2, 4, 5, 3, 
                         6, 0, 2, 4, 
                         2, 5, 6, 7,
                         3, 1, 5, 7};

/*
  Retrieve the x/y location on the face based on the u/v coordinates
*/
void get_location( int face, double u, double v, 
                   double *X ){
  double N[4];
  N[0] = (1.0 - u)*(1.0 - v);
  N[1] = u*(1.0 - v);
  N[2] = (1.0 - u)*v;
  N[3] = u*v;

  X[0] = (N[0]*test_Xpts[3*test_conn[4*face]] +
          N[1]*test_Xpts[3*test_conn[4*face+1]] +
          N[2]*test_Xpts[3*test_conn[4*face+2]] +
          N[3]*test_Xpts[3*test_conn[4*face+3]]);
  X[1] = (N[0]*test_Xpts[3*test_conn[4*face]+1] +
          N[1]*test_Xpts[3*test_conn[4*face+1]+1] +
          N[2]*test_Xpts[3*test_conn[4*face+2]+1] +
          N[3]*test_Xpts[3*test_conn[4*face+3]+1]);
  X[2] = (N[0]*test_Xpts[3*test_conn[4*face]+2] +
          N[1]*test_Xpts[3*test_conn[4*face+1]+2] +
          N[2]*test_Xpts[3*test_conn[4*face+2]+2] +
          N[3]*test_Xpts[3*test_conn[4*face+3]+2]);
}

/*
  Compute the Jacobian transformation at a point within the element
*/
void computeJacobianTrans( const TacsScalar Xpts[],
                           const double Na[], const double Nb[], 
                           TacsScalar Xd[], TacsScalar J[] ){
  memset(Xd, 0, 9*sizeof(TacsScalar));

  // Compute the derivative along the coordinate directions
  const double *na = Na, *nb = Nb;
  const TacsScalar *X = Xpts;
  for ( int i = 0; i < 9; i++ ){
    Xd[0] += X[0]*na[0];
    Xd[1] += X[1]*na[0];
    Xd[2] += X[2]*na[0];

    Xd[3] += X[0]*nb[0];
    Xd[4] += X[1]*nb[0];
    Xd[5] += X[2]*nb[0];

    na++;  nb++;
    X += 3;
  }

  // Compute the cross-product with the normal
  Tensor::crossProduct3D(&Xd[6], &Xd[0], &Xd[3]);
  Tensor::normalize3D(&Xd[6]);

  // Compute the transpose of the Jacobian transformation
  FElibrary::jacobian3d(Xd, J);
}

/*
  Evaluate the enrichment functions
*/
void evalEnrichmentFuncs( const double pt[], double N[] ){
  // Compute the cubic enrichment shape functions along the two
  // coordinate directions
  double ca = (1.0 + pt[0])*pt[0]*(1.0 - pt[0]);
  double cb = (1.0 + pt[1])*pt[1]*(1.0 - pt[1]);

  // Set the shape functions themselves
  N[0] = ca;
  N[1] = pt[1]*ca;
  N[2] = pt[1]*pt[1]*ca;
  N[3] = cb;
  N[4] = pt[0]*cb;
  N[5] = pt[0]*pt[0]*cb;
  N[6] = ca*cb;
}

/*
  The enriched shape functions for the tensor-product quadratic
  Lagrange shape functions.
*/
void evalEnrichmentFuncs( const double pt[],
                          double N[], double Na[], double Nb[] ){
  // Compute the cubic enrichment shape functions along the two
  // coordinate directions
  double ca = (1.0 + pt[0])*pt[0]*(1.0 - pt[0]);
  double cb = (1.0 + pt[1])*pt[1]*(1.0 - pt[1]);

  // Compute the derivative of the enrichment functions w.r.t. 
  // the two coordinate directions
  double da = 1.0 - 3*pt[0]*pt[0];
  double db = 1.0 - 3*pt[1]*pt[1];

  // Set the shape functions themselves
  N[0] = ca;
  N[1] = pt[1]*ca;
  N[2] = pt[1]*pt[1]*ca;
  N[3] = cb;
  N[4] = pt[0]*cb;
  N[5] = pt[0]*pt[0]*cb;
  N[6] = ca*cb;

  // Set the derivatives of the enrichment functions with respect to
  // the first and second coordinate directions
  Na[0] = da;
  Na[1] = pt[1]*da;
  Na[2] = pt[1]*pt[1]*da;
  Na[3] = 0.0;
  Na[4] = cb;
  Na[5] = 2.0*pt[0]*cb;
  Na[6] = da*cb;
  
  Nb[0] = 0.0;
  Nb[1] = ca;
  Nb[2] = 2.0*pt[1]*ca;
  Nb[3] = db;
  Nb[4] = pt[0]*db;
  Nb[5] = pt[0]*pt[0]*db;
  Nb[6] = ca*db;
}

/*
  Given the values of the derivatives of one component of the
  displacement or rotation field at the nodes, compute the
  reconstruction over the element by solving a least-squares problem

  input:
  Xpts:    the element node locations
  uvals:   the solution at the nodes
  uderiv:  the derivative of the solution in x/y/z at the nodes
  
  output:
  ubar:    the values of the coefficients on the enrichment functions
*/
void computeElemRecon( const TacsScalar Xpts[],
                       const TacsScalar uvals[],
                       const TacsScalar uderiv[],
                       TacsScalar ubar[] ){
  // Set up the least squares problem at the nodes
  TacsScalar A[18*7], b[18*6];
  int nrhs = 6;

  memset(b, 0, sizeof(b));
  memset(A, 0, sizeof(A));

  const double wvals[] = {0.5, 1.0, 0.5};

  for ( int c = 0, jj = 0; jj < 3; jj++ ){
    for ( int ii = 0; ii < 3; ii++, c += 2 ){
      // Set the parametric location within the element
      double pt[2];
      pt[0] = -1.0 + 1.0*ii;
      pt[1] = -1.0 + 1.0*jj;

      // Compute the quadratic shape functions at this point
      double N[9], Na[9], Nb[9];
      FElibrary::biLagrangeSF(N, Na, Nb, pt, 3);
      
      // Evaluate the Jacobian transformation at this point
      TacsScalar Xd[9], J[9];
      computeJacobianTrans(Xpts, Na, Nb, Xd, J);

      // Normalize the first direction
      TacsScalar d1[3], d2[3];
      d1[0] = Xd[0];  d1[1] = Xd[1];  d1[2] = Xd[2];
      Tensor::normalize3D(d1);

      // Compute d2 = n x d1
      Tensor::crossProduct3D(d2, &Xd[6], d1);

      // First, compute the contributions to the righ-hand-side. The
      // right vector contains the difference between the prescribed
      // derivative and the contribution to the derivative from the
      // quadratic shape function terms
      const TacsScalar *ud = &uderiv[18*(ii + 3*jj)];
      for ( int k = 0; k < nrhs; k++ ){
        b[18*k+c] = 
          wvals[ii]*wvals[jj]*(d1[0]*ud[0] + d1[1]*ud[1] + d1[2]*ud[2]);
        b[18*k+c+1] = 
          wvals[ii]*wvals[jj]*(d2[0]*ud[0] + d2[1]*ud[1] + d2[2]*ud[2]);
        ud += 3;
      }

      // Compute the derivatives from the interpolated solution
      TacsScalar Ud[6*2];
      memset(Ud, 0, sizeof(Ud));
      for ( int i = 0; i < 9; i++ ){
        for ( int k = 0; k < 6; k++ ){
          Ud[2*k] += uvals[6*i + k]*Na[i];
          Ud[2*k+1] += uvals[6*i + k]*Nb[i];
        }
      }
      
      // Set the right-hand side
      for ( int k = 0; k < nrhs; k++ ){
        TacsScalar d[3];
        d[0] = Ud[2*k]*J[0] + Ud[2*k+1]*J[1];
        d[1] = Ud[2*k]*J[3] + Ud[2*k+1]*J[4];
        d[2] = Ud[2*k]*J[6] + Ud[2*k+1]*J[7];

        b[18*k+c] -= 
          wvals[ii]*wvals[jj]*(d1[0]*d[0] + d1[1]*d[1] + d1[2]*d[2]);
        b[18*k+c+1] -= 
          wvals[ii]*wvals[jj]*(d2[0]*d[0] + d2[1]*d[1] + d2[2]*d[2]);
      }
      
      // Now, evaluate the terms for the left-hand-side
      // that contribute to the
      double Nr[7], Nar[7], Nbr[7];
      evalEnrichmentFuncs(pt, Nr, Nar, Nbr);

      // Add the contributions to the the enricment 
      for ( int i = 0; i < 7; i++ ){
        // Evaluate the
        TacsScalar d[3];
        d[0] = Nar[i]*J[0] + Nbr[i]*J[1];
        d[1] = Nar[i]*J[3] + Nbr[i]*J[4];
        d[2] = Nar[i]*J[6] + Nbr[i]*J[7];

        A[18*i+c] = 
          wvals[ii]*wvals[jj]*(d1[0]*d[0] + d1[1]*d[1] + d1[2]*d[2]);
        A[18*i+c+1] = 
          wvals[ii]*wvals[jj]*(d2[0]*d[0] + d2[1]*d[1] + d2[2]*d[2]);
      }
    }
  }

  // Singular values
  TacsScalar s[7];
  int m = 18, n = 7;
  double rcond = -1.0;
  int rank;
  int lwork = 10*18;
  TacsScalar work[10*18];
  int info;

  // Using LAPACK, compute the least squares solution
  LAPACKdgelss(&m, &n, &nrhs, A, &m, b, &m, s, 
               &rcond, &rank, work, &lwork, &info);

  // Copy over the ubar solution
  for ( int i = 0; i < n; i++ ){
    for ( int j = 0; j < nrhs; j++ ){
      ubar[nrhs*i + j] = b[m*j + i];
    }
  }
}

/*
  Compute the local derivative weights
*/
void computeLocalWeights( TACSAssembler *tacs, 
                          TacsScalar **_wlocal ){
  // Create the weight vector - the weights on the averages
  // Set the weights on each of the nodes
  BVec *weights = new BVec(tacs->getVarMap(), 1);
  weights->incref();

  // Retrieve the distributed object from TACSAssembler
  BVecDistribute *vecDist = tacs->getBVecDistribute();

  // Allocate space for the local weights: The number of independent
  // and dependent nodes
  int size = tacs->getNumNodes() + tacs->getNumDependentNodes();
  TacsScalar *wlocal = new TacsScalar[ size ];
  memset(wlocal, 0, size*sizeof(TacsScalar));

  // Set the local element weights - this assumes that all the
  // elements are bi-quadratic 9-noded elements
  TacsScalar welem[9]; 
  for ( int i = 0; i < 9; i++ ){
    welem[i] = 1.0;
  }

  // Add unit weights to all the elements. This will sum up
  // the number of times each node is referenced
  int nelems = tacs->getNumElements();
  for ( int i = 0; i < nelems; i++ ){
    tacs->addValues(1, i, welem, wlocal);
  }

  // Add in the dependent weight values
  tacs->addDependentResidual(1, wlocal);

  // Add all the weigths together
  vecDist->beginReverse(wlocal, weights, BVecDistribute::ADD);
  vecDist->endReverse(wlocal, weights, BVecDistribute::ADD);

  // Send the weights back to the local weights values
  vecDist->beginForward(weights, wlocal);
  vecDist->endForward(weights, wlocal);

  // Set the dependent values
  tacs->setDependentVariables(1, wlocal);

  // Free the BVec object
  weights->decref();

  // Return the local weight values
  *_wlocal = wlocal;
}

/*
  Given the input solution, compute and set the derivatives 
  in the vector ux. Note that this vector must be 3*

  input:
  TACSAssembler:  the TACSAssembler object
  uvec:           the solution vector
  wlocal:         the local element-weight values

  output:
  uderiv:         the approximate derivatives at the nodes
*/
void computeNodeDeriv( TACSAssembler *tacs, BVec *uvec, 
                       const TacsScalar *wlocal, BVec *uderiv ){
  // The number of independent + dependent nodes
  int size = tacs->getNumNodes() + tacs->getNumDependentNodes();

  // The number of variables at each node
  int vars_per_node = tacs->getVarsPerNode();
  int deriv_per_node = 3*vars_per_node;

  // Get the number of elements
  int nelems = tacs->getNumElements();

  // Retrieve the distributed object from TACSAssembler
  BVecDistribute *vecDist = tacs->getBVecDistribute();

  // Set the local adjoint variables
  TacsScalar *ulocal = new TacsScalar[ vars_per_node*size ];
  vecDist->beginForward(uvec, ulocal);
  vecDist->endForward(uvec, ulocal);
  tacs->setDependentVariables(vars_per_node, ulocal);
  
  // Allocate space for the local variables
  TacsScalar *dlocal = new TacsScalar[ deriv_per_node*size ];
  memset(dlocal, 0, deriv_per_node*size*sizeof(TacsScalar));

  // Allocate space for the element-wise values and derivatives
  TacsScalar *uelem = new TacsScalar[ 9*vars_per_node ];
  TacsScalar *delem = new TacsScalar[ 9*deriv_per_node ];

  // Perform the reconstruction for the local
  for ( int elem = 0; elem < nelems; elem++ ){
    // Get the local weight values
    TacsScalar welem[9];
    tacs->getValues(1, elem, wlocal, welem);

    // Get the local element adjoint variables
    tacs->getValues(vars_per_node, elem, ulocal, uelem);

    // Get the node locations for the element
    TacsScalar Xpts[3*9];
    tacs->getElement(elem, Xpts, NULL, NULL, NULL);
    
    // Compute the derivative of the 6 components of the adjoint
    // variables along the 3-coordinate directions
    TacsScalar *d = delem;

    // Compute the contributions to the derivative from this side of
    // the element    
    for ( int jj = 0; jj < 3; jj++ ){
      for ( int ii = 0; ii < 3; ii++ ){
        double pt[2];
        pt[0] = -1.0 + 1.0*ii;
        pt[1] = -1.0 + 1.0*jj;

        // Evaluate the the quadratic shape functions at this point
        double N[9], Na[9], Nb[9];
        FElibrary::biLagrangeSF(N, Na, Nb, pt, 3);
      
        // Evaluate the Jacobian transformation at this point
        TacsScalar Xd[9], J[9];
        computeJacobianTrans(Xpts, Na, Nb, Xd, J);

        // Compute the derivatives from the interpolated solution
        TacsScalar Ud[6*2];
        memset(Ud, 0, sizeof(Ud));
        for ( int i = 0; i < 9; i++ ){
          for ( int k = 0; k < 6; k++ ){
            Ud[2*k] += uelem[6*i + k]*Na[i];
            Ud[2*k+1] += uelem[6*i + k]*Nb[i];
          }
        }

        // Evaluate the x/y/z derivatives of each value at the
        // coordinate locations
        TacsScalar winv = 1.0/welem[3*jj + ii];
        for ( int k = 0; k < vars_per_node; k++ ){
          d[0] = winv*(Ud[2*k]*J[0] + Ud[2*k+1]*J[1]);
          d[1] = winv*(Ud[2*k]*J[3] + Ud[2*k+1]*J[4]);
          d[2] = winv*(Ud[2*k]*J[6] + Ud[2*k+1]*J[7]);
          d += 3;
        }
      }
    }

    // Add the values
    tacs->addValues(deriv_per_node, elem, delem, dlocal);
  }

  // Free the element values
  delete [] uelem;
  delete [] delem;

  // Set the dependent residual values
  tacs->addDependentResidual(deriv_per_node, dlocal);
  
  // Add all the weigths together
  vecDist->beginReverse(dlocal, uderiv, BVecDistribute::ADD);
  vecDist->endReverse(dlocal, uderiv, BVecDistribute::ADD);

  // Free any memory that is not required
  delete [] ulocal;
  delete [] dlocal;
}

/*
  Refine the mesh with the given refinement specified on each
  processor

  This function takes the relative refinement on each local processor,
  and reduces this to the root processor. The code then computes the
  local elements which need to be refined and then applies the
  refinement to the quadtree forrest. This quadtree forrest can then
  be used for creating the next mesh.

  input:
  comm:        the MPI communicator
  forrest:     the quadtree forrest
  nelems:      the local number of elements
  ntotal:      the total number of elements
  partition:   the element partition from the TACSCreator object
  min_refine:  the minimum quadrant refinement level
  max_refine:  the maximum quadrant refinement level
*/
void refineQuadMesh( MPI_Comm comm, TMRQuadForrest *forrest,
                     int *refine_local,
                     int nelems, int ntotal,
                     const int *partition, 
                     const int min_refine, const int max_refine ){
  // Set the rank and size
  int mpi_rank, mpi_size;
  MPI_Comm_rank(comm, &mpi_rank);
  MPI_Comm_size(comm, &mpi_size);

  // Gather the number of elements from each processor
  int *refine = NULL;
  int *elems_per_proc = NULL;
  int *elem_proc_offset = NULL;
  if (mpi_rank == 0){
    elems_per_proc = new int[ mpi_size ];
    elem_proc_offset = new int[ mpi_size ];
    refine = new int[ ntotal ];
  }
  
  // Gather the number of elements from each processor to the root
  // processor
  MPI_Gather(&nelems, 1, MPI_INT, elems_per_proc, 1, MPI_INT, 0, comm);

  if (mpi_rank == 0){
    elem_proc_offset[0] = 0;
    for ( int i = 0; i < mpi_size-1; i++ ){
      elem_proc_offset[i+1] = elem_proc_offset[i] + elems_per_proc[i];
    }
  }
  
  // Gather the element strain energy values to the root processor
  MPI_Gatherv(refine_local, nelems, MPI_INT, 
              refine, elems_per_proc, elem_proc_offset, MPI_INT, 
              0, comm);
 
  if (mpi_rank == 0){    
    // Set the refinement
    int *refinement = new int[ ntotal ];
    memset(refinement, 0, ntotal*sizeof(int));
        
    for ( int i = 0; i < ntotal; i++ ){
      // Get the partition that this element is on
      int proc = partition[i];
      
      // Retrieve the offset from the processor offset
      int index = elem_proc_offset[proc];
      elem_proc_offset[proc]++;
      
      if (refine[index] == 1){
        refinement[i] = 1;
      }
      else {
        refinement[i] = 0;
      }
    }
    
    // Free the strain energy information and offset values
    delete [] elems_per_proc;
    delete [] elem_proc_offset;
    delete [] refine;

    // Set the refinement on each of the quadtrees
    TMRQuadtree **trees;
    int ntrees = forrest->getQuadtrees(&trees);
    
    // Loop through all trees and refine them
    for ( int i = 0, offset = 0; i < ntrees; i++ ){
      int nlocal = trees[i]->getNumElements();
      trees[i]->refine(&refinement[offset], min_refine, max_refine);
      offset += nlocal;
    }
    
    delete [] refinement;
  }
}

/*
  The following function performs a mesh refinement based on a strain
  energy criteria. It is based on the following relationship for
  linear finite-element analysis

  a(u-uh,u-uh) = a(u,u) - a(uh,uh) 

  where a(u,u) is the bilinear strain energy functional, u is the
  exact solution, and uh is the discretized solution at any mesh
  level. This relies on the relationship that a(uh, u - uh) = 0 which
  is satisfied due to the method of Galerkin/Ritz.

  The following function computes a localized error indicator using
  the element-wise strain energy. The code computes a higher-order
  reconstructed solution using a cubic enrichment functions. These
  enrichment functions expand the original solution space and are
  computed based on a least-squares approximation with nodal gradient
  values. The localized error indicator is evaluated as follows:

  err = [sum_{i=1}^{4} ae(uCe, uCe) ] - ae(ue, ue)

  where uCe is the element-wise cuibc element reconstruction projected
  onto a uniformly refined mesh.

  input:
  tacs:        the TACSAssembler object
  uvec:        the solution variables in the mesh
  partition:   the element partition
  forrest:     the forrest of quadtrees 
  min_refine:  minimum refinement
  max_refnie:  maximum refinement

  returns:     the strain energy error
*/
TacsScalar strainEnergyRefine( TACSAssembler *tacs,
                               BVec *uvec,
                               const int *partition,
                               TMRQuadForrest *forrest, 
                               int min_refine, int max_refine ){
  // Get the communicator
  MPI_Comm comm = tacs->getMPIComm();
  
  // Ensure that the local variables have been set
  tacs->setVariables(uvec);

  // Perform a local refinement of the nodes based on the strain energy
  // within each element
  int nelems = tacs->getNumElements();
  TacsScalar *SE_error = new TacsScalar[ nelems ];
  
  // Compute the reconstructed solution
  TacsScalar *wlocal;
  computeLocalWeights(tacs, &wlocal);
  BVec *uderiv = new BVec(tacs->getVarMap(), 
                          3*tacs->getVarsPerNode());
  uderiv->incref();

  // Compute the nodal derivatives
  computeNodeDeriv(tacs, uvec, wlocal, uderiv);

  // Set the local values of the derivatives
  int vars_per_node = tacs->getVarsPerNode();
  int deriv_per_node = 3*vars_per_node;
  int size = tacs->getNumNodes() + tacs->getNumDependentNodes();

  // Distribute the components to the local vector
  TacsScalar *dlocal = new TacsScalar[ deriv_per_node*size ];
  BVecDistribute *vecDist = tacs->getBVecDistribute();
  vecDist->beginForward(uderiv, dlocal);
  vecDist->endForward(uderiv, dlocal);
  tacs->setDependentVariables(deriv_per_node, dlocal);

  // Zero the time-derivatives: this assumes a steady-state
  TacsScalar dvars[6*9], ddvars[6*9];
  memset(dvars, 0, sizeof(dvars));
  memset(ddvars, 0, sizeof(ddvars));

  // Keep track of the total error
  TacsScalar SE_total_error = 0.0;

  // For each element in the mesh, compute the original strain energy
  for ( int i = 0; i < nelems; i++ ){
    // Set the simulation time
    double time = 0.0;

    // Get the node locations and variables
    TacsScalar Xpts[3*9], uelem[6*9];
    TACSElement *elem = tacs->getElement(i, Xpts, uelem, NULL, NULL);
    
    // Evaluate the element residual
    TacsScalar res[6*9];
    memset(res, 0, elem->numVariables()*sizeof(TacsScalar));
    elem->addResidual(time, res, Xpts, uelem, dvars, ddvars);
    
    // Take the inner product to get the strain energy
    SE_error[i] = 0.0;
    for ( int j = 0; j < elem->numVariables(); j++ ){
      SE_error[i] += res[j]*uelem[j];
    }

    // Compute the solution on the refined mesh
    TacsScalar delem[18*9];
    tacs->getValues(deriv_per_node, i, dlocal, delem);

    // 7 enrichment functions for each degree of freedom
    TacsScalar ubar[7*6];
    computeElemRecon(Xpts, uelem, delem, ubar);

    TacsScalar SE_refine = 0.0;
    for ( int jj = 0; jj < 2; jj++ ){
      for ( int ii = 0; ii < 2; ii++ ){
        TacsScalar rXpts[3*9], ruelem[6*9];
        memset(rXpts, 0, 3*9*sizeof(TacsScalar));
        memset(ruelem, 0, 6*9*sizeof(TacsScalar));

        for ( int m = 0; m < 3; m++ ){
          for ( int n = 0; n < 3; n++ ){
            double pt[2];
            pt[0] = -1.0 + 0.5*(2*ii + n);
            pt[1] = -1.0 + 0.5*(2*jj + m);

            // Evaluate the locations of the new nodes
            double N[9], Nr[7];
            FElibrary::biLagrangeSF(N, pt, 3);
            evalEnrichmentFuncs(pt, Nr);
            
            // Set the values of the variables at this point
            for ( int k = 0; k < 9; k++ ){
              rXpts[3*(n + 3*m)] += Xpts[3*k]*N[k];
              rXpts[3*(n + 3*m)+1] += Xpts[3*k+1]*N[k];
              rXpts[3*(n + 3*m)+2] += Xpts[3*k+2]*N[k];

              // Evaluate the interpolation part of the reconstruction
              ruelem[6*(n + 3*m)] += uelem[6*k]*N[k];
              ruelem[6*(n + 3*m)+1] += uelem[6*k+1]*N[k];
              ruelem[6*(n + 3*m)+2] += uelem[6*k+2]*N[k];
              ruelem[6*(n + 3*m)+3] += uelem[6*k+3]*N[k];
              ruelem[6*(n + 3*m)+4] += uelem[6*k+4]*N[k];
              ruelem[6*(n + 3*m)+5] += uelem[6*k+5]*N[k];
            }

            // Add the portion from the enrichment functions
            for ( int k = 0; k < 7; k++ ){
              // Evaluate the interpolation part of the reconstruction
              ruelem[6*(n + 3*m)] += ubar[6*k]*Nr[k];
              ruelem[6*(n + 3*m)+1] += ubar[6*k+1]*Nr[k];
              ruelem[6*(n + 3*m)+2] += ubar[6*k+2]*Nr[k];
              ruelem[6*(n + 3*m)+3] += ubar[6*k+3]*Nr[k];
              ruelem[6*(n + 3*m)+4] += ubar[6*k+4]*Nr[k];
              ruelem[6*(n + 3*m)+5] += ubar[6*k+5]*Nr[k];
            }
          }
        }

        // Compute the element residual
        memset(res, 0, elem->numVariables()*sizeof(TacsScalar));
        elem->addResidual(time, res, rXpts, ruelem, dvars, ddvars);
        
        // Take the inner product to get the strain energy
        for ( int j = 0; j < elem->numVariables(); j++ ){
          SE_refine += res[j]*ruelem[j];
        }
      }
    }
    // SE_refine - SE_error should always be a positive quantity
    SE_error[i] = fabs(SE_refine - SE_error[i]);

    // Add up th etotal error
    SE_total_error += SE_error[i];
  }

  // Count up the total strain energy 
  TacsScalar SE_temp = 0.0;
  MPI_Allreduce(&SE_total_error, &SE_temp, 1, TACS_MPI_TYPE, MPI_SUM, comm);
  SE_total_error = SE_temp;

  // Count up the total number of elements
  int ntotal = 0;
  MPI_Allreduce(&nelems, &ntotal, 1, MPI_INT, MPI_SUM, comm);

  int rank;
  MPI_Comm_rank(comm, &rank);
  if (rank == 0){
    printf("SE_error_total = %15.5e\n", SE_total_error);
  }

  // Go through and flag which element should be refined
  int *refine_local = new int[ nelems ];
  memset(refine_local, 0, nelems*sizeof(int));
  for ( int i = 0; i < nelems; i++ ){
    if (SE_error[i] >= SE_total_error/ntotal){
      refine_local[i] = 1;
    }
  }

  // Free some of the data that is no longer required
  delete [] SE_error;
  delete [] wlocal;
  delete [] dlocal;
  uderiv->decref();

  // refine the quadrant mesh based on the local refinement values
  refineQuadMesh(comm, forrest, refine_local, nelems, ntotal,
                 partition, min_refine, max_refine);

  delete [] refine_local;

  // Return the error
  return SE_total_error;
}

/*
  Refine the mesh using the adjoint solution

  This function computes the refinement based on the following
  localized error measure:

  er = component-wise[C*psi - Q*psi, R(C*u) - R(Q*u)]

  



  input:
  tacs:        the TACSAssembler object
  uvec:        the solution variables
  adj:         the adjoint solution variables
  partition:   the element partition
  forrest:     the forrest of quadtrees 
  min_refine:  minimum refinement
  max_refnie:  maximum refinement
*/
static TACSElement *global_trac;

TacsScalar adjointRefine( TACSAssembler *tacs,
                          BVec *uvec, BVec *adjvec,
                          const int *partition,
                          TMRQuadForrest *forrest,
                          int min_refine, int max_refine ){
  // Get the communicator
  MPI_Comm comm = tacs->getMPIComm();
  
  // Ensure that the local variables have been set
  tacs->setVariables(uvec);

  // Perform a local refinement of the nodes based on the strain energy
  // within each element
  int nelems = tacs->getNumElements();
  
  // Compute the reconstructed solution
  TacsScalar *wlocal;
  computeLocalWeights(tacs, &wlocal);
  BVec *uderiv = new BVec(tacs->getVarMap(), 
                          3*tacs->getVarsPerNode());
  BVec *adjderiv = new BVec(tacs->getVarMap(), 
                            3*tacs->getVarsPerNode());
  uderiv->incref();
  adjderiv->incref();

  // Compute the nodal derivatives
  computeNodeDeriv(tacs, uvec, wlocal, uderiv);
  computeNodeDeriv(tacs, adjvec, wlocal, adjderiv);

  // Set the local values of the derivatives
  int vars_per_node = tacs->getVarsPerNode();
  int deriv_per_node = 3*vars_per_node;
  int size = tacs->getNumNodes() + tacs->getNumDependentNodes();

  // Set the localized error values
  TacsScalar *errlocal = new TacsScalar[ vars_per_node*size ];
  memset(errlocal, 0, vars_per_node*size*sizeof(TacsScalar));

  // Distribute the components to the local vector
  TacsScalar *dlocal = new TacsScalar[ deriv_per_node*size ];
  TacsScalar *adjlocal = new TacsScalar[ vars_per_node*size ];
  TacsScalar *dadjlocal = new TacsScalar[ deriv_per_node*size ];

  // Set the derivatives of the displacements
  BVecDistribute *vecDist = tacs->getBVecDistribute();
  vecDist->beginForward(uderiv, dlocal);
  vecDist->endForward(uderiv, dlocal);
  tacs->setDependentVariables(deriv_per_node, dlocal);

  // Transfer the variables for the adjoint and the derivative of the
  // adjoint
  vecDist->beginForward(adjvec, adjlocal);
  vecDist->endForward(adjvec, adjlocal);
  tacs->setDependentVariables(vars_per_node, adjlocal);

  vecDist->beginForward(adjderiv, dadjlocal);
  vecDist->endForward(adjderiv, dadjlocal);
  tacs->setDependentVariables(deriv_per_node, dadjlocal);

  // Zero the time-derivatives: this assumes a steady-state
  TacsScalar dvars[6*9], ddvars[6*9];
  memset(dvars, 0, sizeof(dvars));
  memset(ddvars, 0, sizeof(ddvars));

  // Keep track of the total error
  TacsScalar total_error = 0.0;

  // For each element in the mesh, compute the original strain energy
  for ( int i = 0; i < nelems; i++ ){
    // Set the simulation time
    double time = 0.0;

    // Get the node locations and variables
    TacsScalar Xpts[3*9], uelem[6*9], aelem[6*9];
    TACSElement *elem = tacs->getElement(i, Xpts, uelem, NULL, NULL);

    // Get the values of the adjoint
    tacs->getValues(vars_per_node, i, adjlocal, aelem);

    // Get the derivative of the solution and the adjoint
    TacsScalar delem[18*9], dadjelem[18*9];
    tacs->getValues(deriv_per_node, i, dlocal, delem);
    tacs->getValues(deriv_per_node, i, dadjlocal, dadjelem);

    // 7 enrichment functions for each degree of freedom
    TacsScalar ubar[7*6], adjbar[7*6];
    computeElemRecon(Xpts, uelem, delem, ubar);
    computeElemRecon(Xpts, aelem, dadjelem, adjbar);

    TacsScalar SE_refine = 0.0;
    for ( int jj = 0; jj < 2; jj++ ){
      for ( int ii = 0; ii < 2; ii++ ){
        // The refined node locations and element variables
        TacsScalar rXpts[3*9], ruelem[6*9];
        memset(rXpts, 0, 3*9*sizeof(TacsScalar));
        memset(ruelem, 0, 6*9*sizeof(TacsScalar));

        // The quadratic and cubuic reconstruction of the adjoint
        TacsScalar qadjelem[6*9], cadjelem[6*9];
        memset(qadjelem, 0, 6*9*sizeof(TacsScalar));

        for ( int m = 0; m < 3; m++ ){
          for ( int n = 0; n < 3; n++ ){
            double pt[2];
            pt[0] = -1.0 + 0.5*(2*ii + n);
            pt[1] = -1.0 + 0.5*(2*jj + m);

            // Evaluate the locations of the new nodes
            double N[9];
            FElibrary::biLagrangeSF(N, pt, 3);
           
            // Set the values of the variables at this point
            for ( int k = 0; k < 9; k++ ){
              rXpts[3*(n + 3*m)] += Xpts[3*k]*N[k];
              rXpts[3*(n + 3*m)+1] += Xpts[3*k+1]*N[k];
              rXpts[3*(n + 3*m)+2] += Xpts[3*k+2]*N[k];

              // Evaluate the interpolation part of the reconstruction
              for ( int kk = 0; kk < 6; kk++ ){
                ruelem[6*(n + 3*m)+kk] += uelem[6*k+kk]*N[k];
                qadjelem[6*(n + 3*m)+kk] += aelem[6*k+kk]*N[k];
              }
            }
          }
        }

        // Compute the quadratic element residual
        TacsScalar resq[6*9];
        memset(resq, 0, elem->numVariables()*sizeof(TacsScalar));
        elem->addResidual(time, resq, rXpts, ruelem, dvars, ddvars);

        // Copy over the quadratic part of the adjoint solution
        memcpy(cadjelem, qadjelem, 6*9*sizeof(TacsScalar));
        for ( int m = 0; m < 3; m++ ){
          for ( int n = 0; n < 3; n++ ){
            double pt[2];
            pt[0] = -1.0 + 0.5*(2*ii + n);
            pt[1] = -1.0 + 0.5*(2*jj + m);

            // Evaluate the locations of the new nodes
            double Nr[7];
            evalEnrichmentFuncs(pt, Nr);
           
            // Set the values of the variables at this point
            for ( int k = 0; k < 7; k++ ){
              // Evaluate the interpolation part of the reconstruction
              for ( int kk = 0; kk < 6; kk++ ){
                ruelem[6*(n + 3*m)+kk] += ubar[6*k+kk]*Nr[k];
                cadjelem[6*(n + 3*m)+kk] += adjbar[6*k+kk]*Nr[k];
              }
            }
          }
        }

        // Compute the cubic part of the residual
        TacsScalar resc[6*9];
        memset(resc, 0, elem->numVariables()*sizeof(TacsScalar));
        elem->addResidual(time, resc, rXpts, ruelem, dvars, ddvars);
        // global_trac->addResidual(time, resc, rXpts, ruelem, dvars, ddvars);

        // Compute the absolute values of the component-wise product
        // of the difference between the residuals of the
        // reconstructed and original solutions and their
        // corresponding adjoint solutions.
        for ( int j = 0; j < elem->numVariables(); j++ ){
          resc[j] = fabs((resc[j] - resq[j])*(cadjelem[j] - qadjelem[j]));
          // resc[j] = fabs(resc[j]*(cadjelem[j] - qadjelem[j]));
          total_error += resc[j];
        }

        // Add the solution error to the residual
        tacs->addValues(vars_per_node, i, resc, errlocal);
      }
    }
  }

  // Set the dependent residual values
  tacs->addDependentResidual(vars_per_node, errlocal);

  // Count up the total strain energy 
  TacsScalar error_temp = 0.0;
  MPI_Allreduce(&total_error, &error_temp, 1, TACS_MPI_TYPE, MPI_SUM, comm);
  total_error = error_temp;

  // Add all the weigths together
  BVec *error = tacs->createVec();
  error->incref();
  vecDist->beginReverse(errlocal, error, BVecDistribute::ADD);
  vecDist->endReverse(errlocal, error, BVecDistribute::ADD);

  // Set the error back into the local values
  vecDist->beginForward(error, errlocal);
  vecDist->endForward(error, errlocal);
  tacs->setDependentVariables(vars_per_node, errlocal);
  
  // Count up the total number of elements
  int ntotal = 0;
  MPI_Allreduce(&nelems, &ntotal, 1, MPI_INT, MPI_SUM, comm);

  // Go through and flag which element should be refined
  int *refine_local = new int[ nelems ];
  memset(refine_local, 0, nelems*sizeof(int));
  for ( int i = 0; i < nelems; i++ ){
    TacsScalar res[6*9];
    tacs->getValues(vars_per_node, i, errlocal, res);

    // Add up the local error
    TacsScalar error = 0.0;
    for ( int j = 0; j < 6*9; j++ ){
      error += res[j];
    }    

    // If the error exceeds the average error, refine it
    if (error >= total_error/ntotal){
      refine_local[i] = 1;
    }
  }

  // Free some of the data that is no longer required
  delete [] wlocal;
  delete [] dlocal;
  delete [] adjlocal;
  delete [] dadjlocal;
  delete [] errlocal;
  uderiv->decref();
  adjderiv->decref();
  error->decref();

  // refine the quadrant mesh based on the local refinement values
  refineQuadMesh(comm, forrest, refine_local, nelems, ntotal,
                 partition, min_refine, max_refine);

  delete [] refine_local;

  // Return the error
  return total_error;

}

/*
  Run an adaptive analysis
*/
int main( int argc, char *argv[] ){
  MPI_Init(&argc, &argv);

  // Set the element order = 3rd order, quadratic elements
  const int ORDER = 3;

  // Set the communicator
  MPI_Comm comm = MPI_COMM_WORLD;

  // Get the MPI communicator rank
  int rank;
  MPI_Comm_rank(comm, &rank);

  // Set the reference direction - this defines the local x-axis
  TacsScalar axis[] = {1.0, 0.0, 0.0};
  
  // Create the material properties
  TacsScalar rho = 2700.0; 
  TacsScalar E = 70e9;
  TacsScalar nu = 0.3;
  TacsScalar kcorr = 5.0/6.0;
  TacsScalar ys = 350e6;
  TacsScalar thickness = 0.0125;
  isoFSDTStiffness *stiff = new isoFSDTStiffness(rho, E, nu, kcorr,
                                                 ys, thickness);
  stiff->setRefAxis(axis);

  // Create the shell element
  TACSElement *element = new MITCShell<ORDER>(stiff);
  element->incref();

  // Test the element implementation on the root processor
  if (rank == 0){
    TestElement *test = new TestElement(element);
    test->incref();
    test->setPrintLevel(2);
    test->testResidual();
    test->testJacobian();
    test->decref();
  }

  // Create the quadtree forrest
  TMRQuadForrest *forrest = NULL;

  // Set the refinement
  int min_refine = 2;
  int max_refine = TMR_MAX_LEVEL;

  if (rank == 0){
    forrest = new TMRQuadForrest(MPI_COMM_SELF);
    
    // Set the connectivity
    int num_nodes = 8;
    int num_faces = 5;
    forrest->setConnectivity(num_nodes, test_conn, num_faces);
    
    // Allocate the trees (random trees for now)
    forrest->createTrees(min_refine);
  }

  for ( int iter = 0; iter < 8; iter++ ){
    // Create the TACSCreator object
    int vars_per_node = 6;
    TACSCreator *creator = new TACSCreator(comm, vars_per_node);
    creator->incref();

    int *partition = NULL;

    if (rank == 0){
      // Balance the forrest so that we can use it!
      forrest->balance(1);
    
      // Create the nodes
      forrest->createNodes(ORDER);
      
      // Extract the mesh
      int nnodes, nelems;
      int *elem_ptr, *elem_conn;
      forrest->getMesh(&nnodes, &nelems,
                       &elem_ptr, &elem_conn);
      
      // Set the element id numbers
      int *elem_id_nums = new int[ nelems ];
      memset(elem_id_nums, 0, nelems*sizeof(int));
      
      // Create the Xpts array
      double *Xpts = new double[ 3*nnodes ];
      
      // Fill in the positions
      TMRQuadtree **quad;
      forrest->getQuadtrees(&quad);
      
      int num_bcs = 0;
      int max_bcs = 10000;
      int *bc_nodes = new int[ max_bcs ];
      
      int num_faces = forrest->getQuadtrees(NULL);
      for ( int face = 0; face < num_faces; face++ ){
        // Retrieve the node quadrants
        TMRQuadrantArray *nodes;
        quad[face]->getNodes(&nodes);
        
        // Get the array of node quadrants from this face
        int size;
        TMRQuadrant *array;
        nodes->getArray(&array, &size);
        
        // Iterate through and evaluate the x/y/z locations
        const double dh = 1.0/(1 << TMR_MAX_LEVEL);
        for ( int i = 0; i < size; i++ ){
          int node = array[i].tag;
          if (node >= 0){
            double u = dh*array[i].x;
            double v = dh*array[i].y;         
            get_location(face, u, v, &Xpts[3*node]);
          
            // Set the boundary conditions based on the spatial location
            if (((Xpts[3*node] < 1e-6 || Xpts[3*node] > 0.999999) ||
                 (Xpts[3*node+1] < 1e-6 || Xpts[3*node+1] > 0.999999))
                && num_bcs < max_bcs){
              bc_nodes[num_bcs] = node;
              num_bcs++;
            }
          }
        }
      }
      
      // Set the connectivity
      creator->setGlobalConnectivity(nnodes, nelems,
                                     elem_ptr, elem_conn,
                                     elem_id_nums);
      delete [] elem_id_nums;
      delete [] elem_ptr; 
      delete [] elem_conn;

      // Set the boundary conditions
      creator->setBoundaryConditions(num_bcs, bc_nodes, NULL, NULL);
      delete [] bc_nodes;

      // Set the nodal locations
      creator->setNodes(Xpts);
      delete [] Xpts;
      
      // Set the dependent nodes
      int num_dep_nodes;
      const int *dep_ptr;
      const int *dep_conn;
      const double *dep_weights;
      forrest->getDependentNodes(&num_dep_nodes, &dep_ptr,
                                 &dep_conn, &dep_weights);
      creator->setDependentNodes(num_dep_nodes, dep_ptr,
                               dep_conn, dep_weights);
    
      // Partition the mesh
      creator->partitionMesh();

      // Copy the element partition
      const int *part;
      partition = new int[ nelems ];
      creator->getElementPartition(&part);
      memcpy(partition, part, nelems*sizeof(int));      
    }
    
    // Set the elements internally within TACS
    creator->setElements(&element, 1);
    
    // Create the TACSAssembler object
    TACSAssembler *tacs = creator->createTACS();
    tacs->incref();

    // Free the creator object - it is not required anymore
    creator->decref();

    // Create the KS function
    double ks_weight = 250.0;
    KSFailure *ks_func = new KSFailure(tacs, ks_weight);
    ks_func->setKSFailureType(KSFailure::CONTINUOUS);
    TACSFunction *ks = ks_func;    
    
    // Create the traction class 
    TacsScalar tx = 0.0, ty = 0.0, tz = 100.0e3;
    TACSElement *trac = new TACSShellTraction<ORDER>(tx, ty, tz);
    global_trac = trac;


    // Create the auxiliary element class
    int nelems = tacs->getNumElements();
    TACSAuxElements *aux = new TACSAuxElements(nelems);
    for ( int i = 0; i < nelems; i++ ){
      aux->addElement(i, trac);
    }
    tacs->setAuxElements(aux);
    
    // Create the preconditioner
    BVec *res = tacs->createVec();
    BVec *ans = tacs->createVec();
    BVec *adjoint = tacs->createVec();
    BVec *tmp = tacs->createVec();
    FEMat *mat = tacs->createFEMat();

    // Increment the reference count to the matrix/vectors
    res->incref();
    ans->incref();
    tmp->incref();
    adjoint->incref();
    mat->incref();
    
    // Allocate the factorization
    int lev = 4500;
    double fill = 10.0;
    int reorder_schur = 1;
    PcScMat *pc = new PcScMat(mat, lev, fill, reorder_schur);
    pc->incref();
    
    // Assemble and factor the stiffness/Jacobian matrix
    double alpha = 1.0, beta = 0.0, gamma = 0.0;
    tacs->assembleJacobian(res, mat, alpha, beta, gamma);

    // Factor the Jacobian matrix
    pc->factor();
    pc->applyFactor(res, ans);
    ans->scale(-1.0);

    // Set the solution
    tacs->setVariables(ans);

    // Check the residual error for the solution
    mat->mult(ans, tmp);
    tmp->axpy(1.0, res);

    // Print the solution norm
    TacsScalar norm = tmp->norm()/res->norm();
    if (rank == 0){
      printf("Solution residual norm: %15.5e\n", norm);
    }

    // Solve the adjoint equation
    TacsScalar fval;
    tacs->evalFunctions(&ks, 1, &fval);
    tacs->evalSVSens(ks, res);
    pc->applyFactor(res, adjoint);

    /*
    tacs->testConstitutive(0, 2);
    tacs->testFunction(ks, 0, 1e-5);
    */
    // tacs->setVariables(adjoint);

    // Create an TACSToFH5 object for writing output to files
    unsigned int write_flag = (TACSElement::OUTPUT_NODES |
                               TACSElement::OUTPUT_DISPLACEMENTS |
                               TACSElement::OUTPUT_STRAINS |
                               TACSElement::OUTPUT_STRESSES |
                               TACSElement::OUTPUT_EXTRAS);
    TACSToFH5 * f5 = new TACSToFH5(tacs, SHELL, write_flag);
    f5->incref();

    // Write the displacements
    char filename[128];
    sprintf(filename, "forrest%d.f5", iter);
    f5->writeToFile(filename);

    tacs->setVariables(adjoint);
    sprintf(filename, "adjoint_forrest%d.f5", iter);
    f5->writeToFile(filename);

    // Delete the viewer

    f5->decref();

    // Set the error
    TacsScalar error_total = 0.0;

    // Perform the refinement
    // strainEnergyRefine(tacs, ans, partition, forrest,
    // min_refine, max_refine);

    error_total = adjointRefine(tacs, ans, adjoint, partition, forrest,
                  min_refine, max_refine);

    if (rank == 0){
      printf("Function value: %20.15e\n", fval);
      printf("Error estimate: %20.15e\n", error_total);
    }

    // Free everything
    delete [] partition;
    tacs->decref();
    res->decref();
    ans->decref();
    tmp->decref();
    mat->decref();
  }

  element->decref();

  MPI_Finalize();
  return (0);
}
