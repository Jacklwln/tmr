import numpy as np
from mpi4py import MPI
from tmr import TMR
from tacs import TACS, elements, constitutive

rae2822_pts = np.array([
   1.0000, 0.0000,
   0.9994, 0.0003,
   0.99759, 0.00069,
   0.99459, 0.00132,
   0.99039, 0.00218,
   0.98502, 0.00326,
   0.97847, 0.00455,
   0.97077, 0.00606,
   0.96194, 0.00775,
   0.952, 0.00964,
   0.94096, 0.0117,
   0.92886, 0.01393,
   0.91574, 0.01627,
   0.9016, 0.01874,
   0.88651, 0.02131,
   0.87048, 0.02397,
   0.85355, 0.0267,
   0.83578, 0.02948,
   0.8172, 0.03231,
   0.79785, 0.03514,
   0.77778, 0.03795,
   0.75705, 0.04075,
   0.7357, 0.04338,
   0.71378, 0.04612,
   0.69134, 0.04857,
   0.66845, 0.05112,
   0.64514, 0.05339,
   0.62149, 0.05547,
   0.59754, 0.05733,
   0.57336, 0.05895,
   0.54901, 0.0603,
   0.52453, 0.06135,
   0.5, 0.06212,
   0.47547, 0.06261,
   0.45099, 0.06286,
   0.42663, 0.06285,
   0.40245, 0.06263,
   0.37851, 0.0622,
   0.35486, 0.06155,
   0.33156, 0.0607,
   0.30866, 0.05967,
   0.28622, 0.05848,
   0.2643, 0.05713,
   0.24295, 0.05556,
   0.22221, 0.05377,
   0.20215, 0.05187,
   0.1828, 0.04987,
   0.16422, 0.04778,
   0.14645, 0.04558,
   0.12952, 0.04321,
   0.11349, 0.04073,
   0.0984, 0.03817,
   0.08427, 0.03552,
   0.07114, 0.0328,
   0.05904, 0.03004,
   0.04801, 0.02726,
   0.03806, 0.02445,
   0.02923, 0.02163,
   0.02153, 0.01875,
   0.01498, 0.01579,
   0.00961, 0.01269,
   0.00541, 0.00945,
   0.00241, 0.00642,
   0.0006, 0.00323,
   0.0000, 0.0000,
   0.0006, -0.00317,
   0.00241, -0.00658,
   0.00541, -0.00957,
   0.00961, -0.01273,
   0.01498, -0.0158,
   0.02153, -0.0188,
   0.02923, -0.0218,
   0.03806, -0.02472,
   0.04801, -0.02761,
   0.05904, -0.03042,
   0.07114, -0.03315,
   0.08427, -0.03584,
   0.0984, -0.03844,
   0.11349, -0.04094,
   0.12952, -0.04333,
   0.14645, -0.04561,
   0.16422, -0.04775,
   0.1828, -0.04977,
   0.20215, -0.05167,
   0.22221, -0.0534,
   0.24295, -0.05498,
   0.2643, -0.05638,
   0.28622, -0.05753,
   0.30866, -0.05843,
   0.33156, -0.059,
   0.35486, -0.05919,
   0.37851, -0.05893,
   0.40245, -0.05817,
   0.42663, -0.05689,
   0.45099, -0.05515,
   0.47547, -0.05297,
   0.5, -0.05044,
   0.52453, -0.04761,
   0.54901, -0.04452,
   0.57336, -0.04127,
   0.59754, -0.03791,
   0.62149, -0.03463,
   0.64514, -0.0311,
   0.66845, -0.0277,
   0.69134, -0.02438,
   0.71378, -0.02118,
   0.7357, -0.01812,
   0.75705, -0.01524,
   0.77778, -0.01256,
   0.79785, -0.01013,
   0.8172, -0.00792,
   0.83578, -0.00594,
   0.85355, -0.00422,
   0.87048, -0.00273,
   0.88651, -0.00149,
   0.9016, -0.00049,
   0.91574, 0.00027,
   0.92886, 0.00081,
   0.94096, 0.00113,
   0.952, 0.00125,
   0.96194, 0.00125,
   0.97077, 0.00113,
   0.97847, 0.00094,
   0.98502, 0.00071,
   0.99039, 0.00048,
   0.99459, 0.00026,
   0.99759, 0.00009,
   0.9994, -0.00001,
   1.000, 0.0000 ])

rae2822_pts = rae2822_pts.reshape((-1,2))

scale = np.array([6.0, 4.0, 3.0, 2.0, 1.0])
zpts = np.array([0.0, 2.0, 5.0, 10.0, 20.0])

lofts = []
for i in xrange(5):
    X = np.zeros((rae2822_pts.shape[0], 3))
    X[:,0] = rae2822_pts[:,0]
    X[:,1] = rae2822_pts[:,1]
    X[:] *= scale[i]
    X[:,2] = zpts[i]

    interp = TMR.CurveInterpolation(X)
    interp.setNumControlPoints(15)
    ku = 4
    lofts.append(interp.createCurve(ku))

kv = 4
lofter = TMR.CurveLofter(lofts)
surface = lofter.createSurface(kv)
surfaces = [surface]

surface.writeToVTK('wing.vtk')

# Create the parametric curves
p1 = TMR.BsplinePcurve(np.array([[.1, 0.], [.4, .0]]))
p2 = TMR.BsplinePcurve(np.array([[.4, 0.], [.4, 1.]]))
p3 = TMR.BsplinePcurve(np.array([[.4, 1.], [.1, 1.]]))
p4 = TMR.BsplinePcurve(np.array([[.1, 1.], [.1, .0]]))

# Create the curves parametrically along the surface
curves = []
curves.append(TMR.CurveFromSurface(surface, p1))
curves.append(TMR.CurveFromSurface(surface, p2))
curves.append(TMR.CurveFromSurface(surface, p3))
curves.append(TMR.CurveFromSurface(surface, p4))

# Create the vertices from the curves
v1 = TMR.VertexFromCurve(curves[0], 0.0)
v2 = TMR.VertexFromCurve(curves[1], 0.0)
v3 = TMR.VertexFromCurve(curves[2], 0.0)
v4 = TMR.VertexFromCurve(curves[3], 0.0)
vertices = [v1, v2, v3, v4]

# Set the vertices in the curves
curves[0].setVertices(v1, v2)
curves[1].setVertices(v2, v3)
curves[2].setVertices(v3, v4)
curves[3].setVertices(v4, v1)

# Set the curve segments around the surface
direct = [1, 1, 1, 1]
surface.addCurveSegment(curves, direct)

geo = TMR.Geometry(vertices, curves, surfaces)
mesh = TMR.Mesh(geo)

# Mesh the geometry
hval = 0.1
mesh.mesh(hval)

# Extract the quadrilaterals/points
pts = mesh.getMeshPoints()
quads = mesh.getMeshConnectivity()
npts = pts.shape[0]
nquads = quads.shape[0]

# Flip the quadrilateral ordering to 
for i in xrange(nquads):
    t = quads[i,2]
    quads[i,2] = quads[i,3]
    quads[i,3] = t

# Flatten the arrays
pts = pts.flatten()
quads = (quads.flatten()).astype(np.intc)

# Set the pointer to the connectivity array
ptr = np.arange(0, 4*nquads+1, 4, dtype=np.intc)
ids = np.arange(0, nquads, dtype=np.intc)

# Create the tacs creator
comm = MPI.COMM_WORLD
vars_per_node = 6
creator = TACS.Creator(comm, vars_per_node)
creator.setGlobalConnectivity(npts, ptr, quads, ids)
creator.setNodes(pts)

# Set constitutive properties
rho = 2500.0 # density, kg/m^3
E = 70e9 # elastic modulus, Pa
nu = 0.3 # poisson's ratio
kcorr = 5.0 / 6.0 # shear correction factor
ys = 350e6 # yield stress, Pa
min_thickness = 0.002
max_thickness = 0.20
thickness = 0.02

stiff = constitutive.isoFSDT(rho, E, nu, kcorr, ys, thickness, 0,
                             min_thickness, max_thickness)

# Set the elements into TACS
elems = []
for i in xrange(nquads):
    elems.append(elements.MITCShell(2, stiff))

# Set the elements
creator.setElements(elems)

# Create the TACS object
tacs = creator.createTACS()

# Visualize it using FH5
flag = (TACS.ToFH5.NODES |
        TACS.ToFH5.DISPLACEMENTS |
        TACS.ToFH5.STRAINS)
f5 = TACS.ToFH5(tacs, TACS.PY_SHELL, flag)
f5.writeToFile('mesh.f5')
