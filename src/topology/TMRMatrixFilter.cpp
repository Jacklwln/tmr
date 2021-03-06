/*
  This file is part of the package TMR for adaptive mesh refinement.

  Copyright (C) 2015 Georgia Tech Research Corporation.
  Additional copyright (C) 2015 Graeme Kennedy.
  All rights reserved.

  TMR is licensed under the Apache License, Version 2.0 (the "License");
  you may not use this software except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "TMRMatrixFilter.h"
#include "TMRMatrixFilterElement.h"
#include "TMR_TACSCreator.h"

/*
  Matrix filter creator classes
*/
class TMRQuadTACSMatrixCreator : public TMRQuadTACSCreator {
public:
  TMRQuadTACSMatrixCreator():
    TMRQuadTACSCreator(NULL){}
  void createElements( int order,
                       TMRQuadForest *forest,
                       int num_elements,
                       TACSElement **elements ){
    TACSElement *elem = NULL;
    if (order == 2){
      elem = new TMRQuadMatrixElement<2>();
    }
    else if (order == 3){
      elem = new TMRQuadMatrixElement<3>();
    }
    else if (order == 4){
      elem = new TMRQuadMatrixElement<4>();
    }
    else if (order == 5){
      elem = new TMRQuadMatrixElement<5>();
    }
    else if (order == 6){
      elem = new TMRQuadMatrixElement<6>();
    }

    for ( int i = 0; i < num_elements; i++ ){
      elements[i] = elem;
    }
  }
};

class TMROctTACSMatrixCreator : public TMROctTACSCreator {
public:
  TMROctTACSMatrixCreator():
    TMROctTACSCreator(NULL){}
  void createElements( int order,
                       TMROctForest *forest,
                       int num_elements,
                       TACSElement **elements ){
    TACSElement *elem = NULL;
    if (order == 2){
      elem = new TMROctMatrixElement<2>();
    }
    else if (order == 3){
      elem = new TMROctMatrixElement<3>();
    }
    else if (order == 4){
      elem = new TMROctMatrixElement<4>();
    }
    else if (order == 5){
      elem = new TMROctMatrixElement<5>();
    }
    else if (order == 6){
      elem = new TMROctMatrixElement<6>();
    }

    for ( int i = 0; i < num_elements; i++ ){
      elements[i] = elem;
    }
  }
};

/*
  Create the filter matrix
*/
TMRMatrixFilter::TMRMatrixFilter( double _s, int _N,
                                  int _nlevels,
                                  TACSAssembler *_tacs[],
                                  TMROctForest *_filter[],
                                  int _vars_per_node ):
  TMRConformFilter(_nlevels, _tacs, _filter, _vars_per_node){
  initialize_matrix(_s, _N, _filter[0], NULL);
}

TMRMatrixFilter::TMRMatrixFilter( double _s, int _N,
                                  int _nlevels,
                                  TACSAssembler *_tacs[],
                                  TMRQuadForest *_filter[],
                                  int _vars_per_node ):
  TMRConformFilter(_nlevels, _tacs, _filter, _vars_per_node){
  initialize_matrix(_s, _N, NULL, _filter[0]);
}

/*
  Initialize the matrix filter.

  This code creates a TACSAssembler object (and frees it), assembles a
  mass matrix, creates the internal variables required for the filter.
*/
void TMRMatrixFilter::initialize_matrix( double _s, int _N,
                                         TMROctForest *oct_forest,
                                         TMRQuadForest *quad_forest ){
  // Create the Assembler object
  TACSAssembler *tacs = NULL;
  if (oct_filter){
    TMROctTACSMatrixCreator *matrix_creator3d =
      new TMROctTACSMatrixCreator();
    matrix_creator3d->incref();

    tacs = matrix_creator3d->createTACS(oct_forest,
                                        TACSAssembler::NATURAL_ORDER);
    tacs->incref();
    matrix_creator3d->decref();
  }
  else {
    TMRQuadTACSMatrixCreator *matrix_creator2d =
      new TMRQuadTACSMatrixCreator();
    matrix_creator2d->incref();

    tacs = matrix_creator2d->createTACS(quad_forest,
                                        TACSAssembler::NATURAL_ORDER);
    tacs->incref();
    matrix_creator2d->decref();
  }

  // Create the matrix
  M = tacs->createMat();
  M->incref();

  // Allocate the vectors needed for the application of the filter
  Dinv = tacs->createVec();
  Tinv = tacs->createVec();
  t1 = tacs->createVec();
  t2 = tacs->createVec();
  t3 = tacs->createVec();
  y1 = tacs->createVec();
  y2 = tacs->createVec();
  Dinv->incref();
  Tinv->incref();
  t1->incref();
  t2->incref();
  t3->incref();
  y1->incref();
  y2->incref();

  // Create a temporary design vector
  temp = createVec();
  temp->incref();

  // Assemble the mass matrix
  tacs->assembleJacobian(1.0, 0.0, 0.0, t2, M);

  // Free this version of TACS - it's not required anymore!
  tacs->decref();

  // Set the number of terms in the M filter
  N = _N;

  // Set the scalar value associated with the filter
  s = _s;
  if (s <= 1.0){
    s = 2.0;
  }

  // Set all vector values to 1.0
  y2->set(1.0);

  // Compute D_{i} = sum_{j=1}^{n} M_{ij}
  M->mult(y2, Dinv);

  // Create the inverse of the diagonal matrix
  TacsScalar *D;
  int size = Dinv->getArray(&D);
  for ( int i = 0; i < size; i++ ){
    if (D[0] != 0.0){
      D[0] = 1.0/D[0];
    }
    else {
      D[0] = 0.0;
    }
    D++;
  }

  // Apply the filter to create the normalization
  Tinv->set(1.0);
  applyFilter(y2, y1);

  // Create the inverse of the T matrix
  TacsScalar *T, *ty;
  size = Tinv->getArray(&T);
  y1->getArray(&ty);
  for ( int i = 0; i < size; i++ ){
    if (ty[0] != 0.0){
      T[0] = 1.0/ty[0];
    }
    else {
      T[0] = 0.0;
    }
    T++;
    ty++;
  }
}

/*
  Destroy the filter matrix
*/
TMRMatrixFilter::~TMRMatrixFilter(){
  t1->decref();
  t2->decref();
  t3->decref();
  M->decref();
  Dinv->decref();
  Tinv->decref();
  y1->decref();
  y2->decref();
  temp->decref();
}

/*
  Compute the action of the filter on the input vector using Horner's
  method

  t1 = 1/s*Dinv*in
  out = t1
  for n in range(N):
  .   out += t1 + 1/s*D^{-1}*M*out
*/
void TMRMatrixFilter::applyFilter( TACSBVec *in, TACSBVec *out ){
  // Compute t1 = 1/s*Dinv*in
  t1->copyValues(in);
  kronecker(Dinv, t1);
  t1->scale(1.0/s);

  // Set out = 1/s*Dinv*in
  out->copyValues(t1);

  // Apply Horner's method
  for ( int n = 0; n < N; n++ ){
    // Compute t2 = 1/s*D^{-1}*M*out
    M->mult(out, t2);
    kronecker(Dinv, t2);
    out->axpy(1.0/s, t2);

    // Compute out = t1 + 1/s*D^{-1}*M*out
    out->axpy(1.0, t1);
  }

  // Multiply by Tinv
  kronecker(Tinv, out);
}

/*
  Compute the transpose of the filter operation
*/
void TMRMatrixFilter::applyTranspose( TACSBVec *in, TACSBVec *out ){
  t1->copyValues(in);
  kronecker(Tinv, t1);

  // Copy the values from t1 to the out vector
  out->copyValues(t1);

  // Apply Horner's method
  for ( int n = 0; n < N; n++ ){
    // Compute M*D^{-1}*out
    kronecker(Dinv, out, t2);
    M->mult(t2, t3);
    out->axpy(1.0/s, t3);

    // Compute out = t1 + 1/s*M*D^{-1}*out
    out->axpy(1.0, t1);
  }

  // Multiply by 1/s*Dinv
  kronecker(Dinv, out);
  out->scale(1.0/s);
}

/*
  Compute the Kronecker product of the input vectors c and x and store
  the result in either y (if it is provided) or otherwise x

  y = (c o x)
*/
void TMRMatrixFilter::kronecker( TACSBVec *c, TACSBVec *x, TACSBVec *y ){
  if (c && x && y){
    TacsScalar *cvals, *xvals, *yvals;
    int size = c->getArray(&cvals);
    x->getArray(&xvals);
    y->getArray(&yvals);

    for ( int i = 0; i < size; i++ ){
      yvals[0] = cvals[0]*xvals[0];
      yvals++;
      xvals++;
      cvals++;
    }
  }
  else if (c && x){
    TacsScalar *cvals, *xvals;
    int size = c->getArray(&cvals);
    x->getArray(&xvals);

    for ( int i = 0; i < size; i++ ){
      xvals[0] *= cvals[0];
      xvals++;
      cvals++;
    }
  }
}

/*
  Set the design variables for each level
*/
void TMRMatrixFilter::setDesignVars( TACSBVec *xvec ){
  if (getVarsPerNode() == 1){
    applyFilter(xvec, x[0]);
  }
  else {
    const int vpn = getVarsPerNode();

    for ( int k = 0; k < vpn; k++ ){
      TacsScalar *xin, *xout;
      xvec->getArray(&xin);
      x[0]->getArray(&xout);

      // Set the pointers offset by the vars per node
      xin = &xin[k];
      xout = &xout[k];

      // Copy the values to the input vector (y1)
      TacsScalar *yin;
      int size = y1->getArray(&yin);
      for ( int i = 0; i < size; i++ ){
        yin[0] = xin[0];
        yin++;
        xin += vpn;
      }

      // Apply the matrix filter from the input y1 to the
      // output y2
      applyFilter(y1, y2);

      // Copy the values from y2 to the output vector x[0]
      TacsScalar *yout;
      y2->getArray(&yout);
      for ( int i = 0; i < size; i++ ){
        xout[0] = yout[0];
        yout++;
        xout += vpn;
      }
    }
  }

  // Distribute the design variable values
  x[0]->beginDistributeValues();
  x[0]->endDistributeValues();

  // Temporarily allocate an array to store the variables
  TacsScalar *xlocal = new TacsScalar[ getMaxNumLocalVars() ];

  // Copy the values to the local array
  int size = getLocalValuesFromBVec(0, x[0], xlocal);
  tacs[0]->setDesignVars(xlocal, size);

  // Set the design variable values on all processors
  for ( int k = 0; k < nlevels-1; k++ ){
    filter_interp[k]->multWeightTranspose(x[k], x[k+1]);

    // Distribute the design variable values
    x[k+1]->beginDistributeValues();
    x[k+1]->endDistributeValues();

    // Set the design variable values
    size = getLocalValuesFromBVec(k+1, x[k+1], xlocal);
    tacs[k+1]->setDesignVars(xlocal, size);
  }

  delete [] xlocal;
}

/*
  Add values to the output TACSBVec
*/
void TMRMatrixFilter::addValues( TacsScalar *xlocal, TACSBVec *vec ){
  temp->zeroEntries();
  setBVecFromLocalValues(0, xlocal, temp, TACS_ADD_VALUES);
  temp->beginSetValues(TACS_ADD_VALUES);
  temp->endSetValues(TACS_ADD_VALUES);

  if (getVarsPerNode() == 1){
    applyTranspose(temp, y1);
    vec->axpy(1.0, y1);
  }
  else {
    const int vpn = getVarsPerNode();

    for ( int k = 0; k < vpn; k++ ){
      TacsScalar *xin, *xout;

      // Get the pointer to the array and offset by vars per node
      temp->getArray(&xin);
      xin = &xin[k];

      // Copy the values to the input vector (y1)
      TacsScalar *yin;
      int size = y1->getArray(&yin);
      for ( int i = 0; i < size; i++ ){
        yin[0] = xin[0];
        yin++;
        xin += vpn;
      }

      // Apply the matrix filter from the input y1 to the
      // output y2
      applyTranspose(y1, y2);

      // Add the values from y2 back to the output vector vec
      vec->getArray(&xout);
      xout = &xout[k];

      TacsScalar *yout;
      y2->getArray(&yout);
      for ( int i = 0; i < size; i++ ){
        xout[0] += yout[0];
        yout++;
        xin += vpn;
      }
    }
  }
}
