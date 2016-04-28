#include "TMRBase.h"

// The TMR data type for MPI useage
MPI_Datatype TMROctant_MPI_type;
MPI_Datatype TMRQuadrant_MPI_type;

/*
  Initialize TMR data type
*/
void TMRInitialize(){
  MPI_Aint offset = 0;
  MPI_Datatype type = MPI_INT32_T;

  // Create the TMROctant data type
  int counts = 5;
  MPI_Type_struct(1, &counts, &offset, &type, 
                  &TMROctant_MPI_type);
  MPI_Type_commit(&TMROctant_MPI_type);

  // Create the TMRQudrant data type
  counts = 4;
  MPI_Type_struct(1, &counts, &offset, &type, 
                  &TMRQuadrant_MPI_type);
  MPI_Type_commit(&TMRQuadrant_MPI_type);
}

/*
  Finalize the TMR data type
*/
void TMRFinalize(){
  MPI_Type_free(&TMROctant_MPI_type);
  MPI_Type_free(&TMRQuadrant_MPI_type);
}