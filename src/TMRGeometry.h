#ifndef TMR_GEOMETRY_H
#define TMR_GEOMETRY_H

/*
  The following header file contains the interface for the geometry/
  topology for the TMR objects. These vertex/edge/surface and volume
  objects are used to map the 

  The vertex, edge, face and volume classes are used in conjunction
  with the TMROct(Quad)Forest class to evaluate nodal locations with
  the mesh. These interfaces are designed to be overriden with an
  external geometry engine.
*/

#include "TMRBase.h"

class TMRSurface;
class TMRCurveMesh;
class TMRSurfaceMesh;

/*
  The vertex class: Note that this is used to store both the
  point and to represent the underlying geometry
*/
class TMRVertex : public TMREntity {
 public:
  TMRVertex(){}
  virtual ~TMRVertex(){}

  // Evalue the point
  virtual int evalPoint( TMRPoint *p ) = 0;
};

/*
  The parametrization for a curve
*/
class TMRCurve : public TMREntity {
 public:
  TMRCurve();
  TMRCurve( TMRVertex *v1, TMRVertex *v2 );
  virtual ~TMRCurve();

  // Get the parameter range for this edge
  virtual void getRange( double *tmin, double *tmax ) = 0;
  
  // Given the parametric point, evaluate the x,y,z location
  virtual int evalPoint( double t, TMRPoint *X ) = 0;

  // Parametrize the curve on the given surface
  virtual int getParamsOnSurface( TMRSurface *surface, double t, 
                                  int dir, double *u, double *v );

  // Given the point, find the parametric location
  virtual int invEvalPoint( TMRPoint X, double *t );

  // Given the parametric point, evaluate the derivative 
  virtual int evalDeriv( double t, TMRPoint *Xt );
  
  // Set/retrive the vertices at the beginning and end of the curve
  void setVertices( TMRVertex *_v1, TMRVertex *_v2 );
  void getVertices( TMRVertex **_v1, TMRVertex **_v2 );

  // Integrate along the edge and return an array containing
  // the parametric locations to provide an even spacing
  double integrate( double t1, double t2, double tol,
                    double **tvals, double **dist, int *nvals );

  // Set/retrieve the mesh
  void setMesh( TMRCurveMesh *_mesh );
  void getMesh( TMRCurveMesh **_mesh );

  // Write the object to the VTK file
  void writeToVTK( const char *filename );
  
 private:
  // Set the step size
  static double deriv_step_size;

  // The start/end vertices of the curve
  TMRVertex *v1, *v2;

  // The mesh for the curve - if it exists
  TMRCurveMesh *mesh;
};

/*
  The parametrization of a surface
*/
class TMRSurface : public TMREntity {
 public:
  TMRSurface();
  virtual ~TMRSurface();

  // Get the parameter range for this surface
  virtual void getRange( double *umin, double *vmin,
                         double *umax, double *vmax ) = 0;
 
  // Given the parametric point, compute the x,y,z location
  virtual int evalPoint( double u, double v, TMRPoint *X ) = 0;
  
  // Perform the inverse evaluation
  virtual int invEvalPoint( TMRPoint p, double *u, double *v ) = 0;

  // Given the parametric point, evaluate the first derivative 
  virtual int evalDeriv( double u, double v, 
                         TMRPoint *Xu, TMRPoint *Xv ) = 0;

  // Add a curve segment. The curve segments must form a closed loop
  // which is checked by the code. The boundary must lie
  // counterclockwise around the surface while holes/cutouts must run
  // clockwise so that the domain always lies to the left of the
  // curve.
  int addCurveSegment( int ncurves, TMRCurve **_curves, 
                       const int _dir[] );
  int getNumSegments();
  int getCurveSegment( int k, int *ncurves, 
                       TMRCurve ***_curves, const int **_dir );

  // Set/retrieve the mesh
  void setMesh( TMRSurfaceMesh *_mesh );
  void getMesh( TMRSurfaceMesh **_mesh );

  // Write the object to the VTK file
  void writeToVTK( const char *filename );

 private:
  // Pointers to the curves that enclose the object. 
  // Note  to the list of curves
  class TMRSegment {
  public:
    int num_curves;
    TMRCurve **curves;
    int *dir;
  }; 
  int max_num_segments;
  int num_segments;
  TMRSegment **segments;

  // The mesh for the curve - if it exists
  TMRSurfaceMesh *mesh;

  // Set the step size
  static double deriv_step_size;
};

/*
  Abstract base class for a pametric curve (u(t), v(t)) which can
  be used to define a curve on a surface
*/
class TMRPcurve : public TMREntity {
 public:
  // Get the parameter range for this edge
  virtual void getRange( double *tmin, double *tmax ) = 0;
  
  // Given the parametric point, evaluate the x,y,z location
  virtual int evalPoint( double t, double *u, double *v ) = 0;

  // Given the parametric point, evaluate the derivative 
  virtual int evalDeriv( double t, double *ut, double *vt ) = 0;
};

/*
  Set the TMRVertex from a point
*/
class TMRVertexFromPoint : public TMRVertex {
 public:
  TMRVertexFromPoint( TMRPoint p );
  ~TMRVertexFromPoint(){}
  int evalPoint( TMRPoint *p );
 private:
  TMRPoint pt;
};

/*
  Set the TMRVertex location based on a parametric location along a
  curve.  

  This takes either a parametric point or does an inverse evaluation
  first to determine the parametric location.
*/
class TMRVertexFromCurve : public TMRVertex {
 public:
  TMRVertexFromCurve( TMRCurve *_curve, double _t );
  TMRVertexFromCurve( TMRCurve *_curve, TMRPoint p );
  ~TMRVertexFromCurve();
  int evalPoint( TMRPoint *p );
  TMRCurve* getCurve();
  double getParamPoint();

 private:
  double t;
  TMRCurve *curve;
};

/*
  Evaluate a vertex location based on its parametric location on
  a surface.
*/
class TMRVertexFromSurface : public TMRVertex {
 public:
  TMRVertexFromSurface( TMRSurface *_surface, double _u, double _v );
  TMRVertexFromSurface( TMRSurface *_surface, TMRPoint p );
  ~TMRVertexFromSurface();
  int evalPoint( TMRPoint *p );

 private:
  double u, v;
  TMRSurface *surface;
};

/*
  The curve parametrized on the surface C(t) = S(u(t), v(t))
*/
class TMRCurveFromSurface : public TMRCurve {
 public:
  TMRCurveFromSurface( TMRSurface *_surface, TMRPcurve *_pcurve );
  ~TMRCurveFromSurface();

  // Get the parameter range for this edge
  void getRange( double *tmin, double *tmax );
  
  // Given the parametric point, evaluate the x,y,z location
  int evalPoint( double t, TMRPoint *X );

  // Parametrize the curve on the given surface
  int getParamsOnSurface( TMRSurface *surface, double t, 
                          int dir, double *u, double *v );

  // Given the point, find the parametric location
  int invEvalPoint( TMRPoint X, double *t );

  // Given the parametric point, evaluate the derivative 
  int evalDeriv( double t, TMRPoint *Xt );

 private:
  TMRSurface *surface;
  TMRPcurve *pcurve;
};

/*
  Project a curve onto a surface and evaluate the surface location
*/
class TMRCurveFromSurfaceProjection : public TMRCurve {
 public:
  TMRCurveFromSurfaceProjection( TMRSurface *_surface, 
                                 TMRCurve *_curve );
  ~TMRCurveFromSurfaceProjection();  
  void getRange( double *tmin, double *tmax );
  int evalPoint( double t, TMRPoint *p );

 private:
  TMRCurve *curve;
  TMRSurface *surface;
};

/*
  Split the curve
*/
class TMRSplitCurve : public TMRCurve {
 public:
  TMRSplitCurve( TMRCurve *_curve, double _t1, double _t2 );
  TMRSplitCurve( TMRCurve *_curve, TMRPoint *p1, TMRPoint *p2 );
  TMRSplitCurve( TMRCurve *_curve, TMRVertex *p1, TMRVertex *p2 );
  TMRSplitCurve();

  // Get the parameter range
  void getRange( double *tmin, double *tmax );

  // Evaluate the point
  int evalPoint( double t, TMRPoint *X );

 private:
  // Evaluate the bspline curve
  double t1, t2;
  TMRCurve *curve;
};

/*
  The TMR Geometry class. 

  This contains the geometry objects -- vertices, curves and 
  surfaces -- that are used to define the geometry of a model.
*/
class TMRGeometry : public TMREntity {
 public:
  TMRGeometry( int _num_vertices, TMRVertex **_vertices, 
               int _num_curves, TMRCurve **_curves,
               int _num_surfaces, TMRSurface **_surfaces );
  ~TMRGeometry();

  void getVertices( int *_num_vertices, TMRVertex ***_vertices );
  void getCurves( int *_num_curves, TMRCurve ***_curves );
  void getSurfaces( int *_num_surfaces, TMRSurface ***_surfaces );

 private:
  int num_vertices, num_curves, num_surfaces;
  TMRVertex **vertices;
  TMRCurve **curves;
  TMRSurface **surfaces;
};

#endif // TMR_GEOMETRY_H
