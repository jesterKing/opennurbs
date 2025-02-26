/* $NoKeywords: $ */
/*
//
// Copyright (c) 1993-2012 Robert McNeel & Associates. All rights reserved.
// OpenNURBS, Rhinoceros, and Rhino3D are registered trademarks of Robert
// McNeel & Associates.
//
// THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
// ALL IMPLIED WARRANTIES OF FITNESS FOR ANY PARTICULAR PURPOSE AND OF
// MERCHANTABILITY ARE HEREBY DISCLAIMED.
//				
// For complete openNURBS copyright information see <http://www.opennurbs.org>.
//
////////////////////////////////////////////////////////////////
*/

#include "opennurbs.h"

#if !defined(ON_COMPILING_OPENNURBS)
// This check is included in all opennurbs source .c and .cpp files to insure
// ON_COMPILING_OPENNURBS is defined when opennurbs source is compiled.
// When opennurbs source is being compiled, ON_COMPILING_OPENNURBS is defined 
// and the opennurbs .h files alter what is declared and how it is declared.
#error ON_COMPILING_OPENNURBS must be defined when compiling opennurbs
#endif

ON_3dSimplex::ON_3dSimplex() { m_n = 0; }

/* this function checks the validity of ClosetPoint results*/
bool ClosestPointIsValid(const ON_ConvexPoly& AHull, const ON_ConvexPoly& BHull,
  ON_4dex Adex, ON_4dex Bdex, ON_4dPoint ABBary, double atmost, ON_TextLog* log = nullptr);


ON_3dSimplex::ON_3dSimplex(const ON_3dPoint& a) { m_n = 1; m_V[0] = a; }
ON_3dSimplex::ON_3dSimplex(const ON_3dPoint& a, const ON_3dPoint& b) { m_n = 2; m_V[0] = a; m_V[1] = b; }
ON_3dSimplex::ON_3dSimplex(const ON_3dPoint& a, const ON_3dPoint& b, const ON_3dPoint& c) { m_n = 3; m_V[0] = a; m_V[1] = b;  m_V[2] = c; }
ON_3dSimplex::ON_3dSimplex(const ON_3dPoint& a, const ON_3dPoint& b, const ON_3dPoint& c, const ON_3dPoint& d) { m_n = 4; m_V[0] = a; m_V[1] = b; m_V[2] = c; m_V[3] = d; }

int ON_3dSimplex::Count() const { return m_n; };

bool ON_3dSimplex::IsValid(double eps) const		// true if the Verticies are affinely independent
{
  bool rc = true;
  if (m_n >= 2)
  {
    ON_3dVector V = m_V[1] - m_V[0];
    if( m_n==2)
      rc = ( V.Length() > eps );
    else
    {
      ON_3dVector W = m_V[2] - m_V[0];
      ON_3dVector X = ON_CrossProduct(V, W);
      // TODO put something smart here....
      if( m_n==3)
        rc = (X.Length() > eps);
      else
      {
        // TODO and  here....
        double triple = X * (m_V[3] - m_V[0]);
        rc = (fabs(triple) > eps);
      }
    }
  }
  return rc;
}

const ON_3dPoint& ON_3dSimplex::operator[](int i) const {
  return *reinterpret_cast<const ON_3dPoint*>(m_V + i);
}

ON_3dPoint& ON_3dSimplex::operator[](int i) {
  return *reinterpret_cast<ON_3dPoint*>(m_V + i);
}


ON_3dPoint ON_3dSimplex::Vertex(int i) const { return ON_3dPoint(m_V[i]); }
ON_3dPoint& ON_3dSimplex::Vertex(int i) { return *reinterpret_cast<ON_3dPoint*>(&m_V[i]); }



ON_3dPoint ON_3dSimplex::Evaluate(const double* b) const
{
  ON_3dVector p(0, 0, 0);
  for (int i = 0; i < m_n; i++)
    p += b[i] * m_V[i];
  return p;
}

ON_3dPoint ON_3dSimplex::Evaluate(const ON_4dPoint& b)  const { return Evaluate(&b.x); }

double ON_3dSimplex::Volume() const
{
  double vol = 0.0;
  int n = Count();
  if (n >= 2)
  {
    ON_3dVector V = m_V[1] - m_V[0];
    if (n == 2) 
      vol = V.Length();
    else
    {
      ON_3dVector X = ON_CrossProduct(V, m_V[2]-m_V[0]);
      if (n == 3)
        vol = 0.5 * X.Length();
      else
        vol = 1.0/6.0 * fabs(X*(m_V[3] - m_V[0]));
    }
  }
  return vol;
}

double ON_3dSimplex::SignedVolume() const
{
  double vol = ON_UNSET_VALUE;
  if (Count() == 3)
  {
    ON_3dVector V = m_V[1] - m_V[0];
    ON_3dVector X = ON_CrossProduct(V, m_V[2] - m_V[0]);
    vol = 1.0 / 6.0 * (X*(m_V[3] - m_V[0]));
   } 
   return vol;
}

ON_BoundingBox ON_3dSimplex::BoundingBox() const
{
  ON_BoundingBox box;
  box.Set(3, false, m_n, 3, m_V[0], false);
  return box;
}

bool  ON_3dSimplex::GetBoundingBox(
  ON_BoundingBox& bbox, int bGrowBox  ) const
{
  return bbox.Set(3, false, m_n, 3, m_V[0], bGrowBox);
}

bool  ON_3dSimplex::GetTightBoundingBox(
  ON_BoundingBox& tight_bbox, bool bGrowBox,
  const ON_Xform* xform  ) const
{
  if (bGrowBox && !tight_bbox.IsValid())
  {
    bGrowBox = false;
  }
  if (!bGrowBox)
  {
    tight_bbox.Destroy();
  }

  int i;
  for (i = 0; i < m_n; i++)
  {
    if (ON_GetPointListBoundingBox(3, 0, m_n, 3, m_V[i], tight_bbox, bGrowBox, xform))
      bGrowBox = true;
  }
  return bGrowBox ? true : false;
}

bool ON_3dSimplex::Transform(
  const ON_Xform& xform)
{
  for (int i = 0; i < m_n; i++)
    m_V[i].Transform(xform);
  return true;
}

bool ON_3dSimplex::Rotate(
  double sin_angle,
  double cos_angle,
  const ON_3dVector& axis_of_rotation,
  const ON_3dPoint& center_of_rotation)
{
  ON_Xform R;
  R.Rotation(sin_angle, cos_angle, axis_of_rotation, center_of_rotation);
  return Transform(R);
}

bool ON_3dSimplex::Rotate(
  double angle_in_radians,
  const ON_3dVector& axis_of_rotation,
  const ON_3dPoint& center_of_rotation )
{
  ON_Xform R;
  R.Rotation(angle_in_radians, axis_of_rotation, center_of_rotation);
  return Transform(R);
}

bool ON_3dSimplex::Translate(  const ON_3dVector& delta )
{
  ON_Xform T = ON_Xform::TranslationTransformation(delta);
  return Transform(T);
}


bool ON_3dSimplex::RemoveVertex(int i)
{
  bool rc = false;
  if (i < m_n)
  {
    m_n--;
    for (/**/; i < m_n; i++)
      m_V[i] = m_V[i + 1];
  }
  return rc;
}

bool ON_3dSimplex::AddVertex(const ON_3dPoint& P)
{
  bool rc = false;
  if (m_n < 4)
  {
    m_V[m_n++] = P;
  }
  return rc;
}


bool ON_3dSimplex::SetVertex(int i, ON_3dPoint P0)
{
  bool rc = false;
  if (i >= 0 && i < Count())
  {
    m_V[i] = P0;
    // todo clear any cashed data
    rc = true;
  }
  return rc;
}

ON_3dVector ON_3dSimplex::Edge(int e0, int e1) const
{
  ON_3dVector V = ON_3dVector::UnsetVector;
  if (e0 < Count() && e1 < Count())
  {
    V = Vertex(e1) - Vertex(e0);
  }
  return V;
}


/*
    This is a carefull implementation of cross product that tries to get an accurate result
*/
ON_3dVector ON_CrossProductwCare(const ON_3dVector& A, ON_3dVector& B)
{
  double norm[3];
  norm[0] = A.MaximumCoordinate();
  norm[1] = B.MaximumCoordinate();
  ON_3dVector AxB = ON_CrossProduct(A, B);
  const double thresh = 1.0e-8;		// sin(A,B) ~< thresh^(1/2) 
  double ab = norm[0] * norm[1];
  double ab2 = ab * ab;
  if (AxB.LengthSquared() < ab2*thresh)
  {
    // TODO - this is a good start but we could do something better...
    ON_3dVector V[3] = { A, B, A - B };
    norm[2] = V[2].MaximumCoordinate();
    int maxi = (norm[0] > norm[1]) ? 0 : 1;
    if (norm[2] < norm[maxi])		// else C is longest so we are done.
    {
      AxB = ON_CrossProduct(V[(maxi + 1) % 3], V[(maxi + 2) % 3]);
      if (maxi == 0)
        AxB =  - AxB;
    }
  }
  return AxB;
}


ON_3dVector ON_3dSimplex::FaceNormal(int noti) const
{
  ON_3dVector N = ON_3dVector::UnsetVector;
  if (Count() == 3 || (Count() == 4 && noti <= 3 && noti >= 0))
  {
    int ind[3] = { 0,1,2 };
    if (Count() == 4 && noti < 3)
    {
      for (int ii = 0; ii < 3; ii++)
        ind[ii] = (noti + 1 + ii) % 4;
    }
    ON_3dVector A = Vertex(ind[1]) - Vertex(ind[0]);
    ON_3dVector B = Vertex(ind[2]) - Vertex(ind[0]);
    N = ON_CrossProductwCare(A, B );
  }
  return N;
}

ON_3dVector ON_3dSimplex::FaceUnitNormal(int noti) const
{
  ON_3dVector N = FaceNormal(noti);
  if (N != ON_3dVector::UnsetVector && N != ON_3dVector::ZeroVector)
    N.Unitize();
  return N;
}

bool ON_3dSimplex::GetClosestPoint(const ON_3dPoint& P0, ON_4dPoint& Bary, double atmost) const
{
  ON_3dVector V = P0;
  ON_3dSimplex Trans;
  bool toofar = ((atmost <= 0.0) ? false : true);
  bool rval = false;
  for (int i = 0; i < Count(); i++)
  {

    Trans.AddVertex(Vertex(i) - V);
    if (toofar && Trans[i].MaximumCoordinate() < .5 * atmost)
      toofar = false;
  }
  if (!toofar)
  {
    rval = Trans.GetClosestPointToOrigin(Bary);
    if (rval && atmost >= 0)
    {
      ON_3dVector CP = Trans.Evaluate(Bary);
      if (CP.LengthSquared() > atmost*atmost)
        rval = false;
    }
  }
  return rval;
}

bool ON_3dSimplex::GetClosestPointToOrigin(ON_4dPoint& Bary) const
{
  bool rc = false;
  if (m_n == 4)
    rc = Closest3plex(Bary);
  else if (m_n == 3)
    rc = Closest2plex(Bary);
  else if (m_n == 2)
    rc = Closest1plex(Bary);
  else if (m_n == 1)
  {
    Bary = ON_4dPoint(1.0, 0, 0, 0);
    rc = true;
  }
  return rc;
}

// return true if a and b are non zero and of same sign
static bool SameSign(double a, double b)
{
  return (a*b) > 0;
}


// closest point to a 1-simplex
bool ON_3dSimplex::Closest1plex(ON_4dPoint& Bary) const
{
  bool rc = false;
  ON_3dVector Del = m_V[1] - m_V[0];
  double Del2 = Del.LengthSquared();
  if (Del2 > 0.0)
  {
    rc = true;
    double dot = -m_V[0] * Del;
    if (dot >= Del2)
      Bary = ON_4dPoint(0, 1, 0, 0);
    else if (dot <= 0)
      Bary = ON_4dPoint(1, 0, 0, 0);
    else
    {
      double b0 = dot / Del2;
      Bary = ON_4dPoint(1 - b0, b0, 0, 0);
    }
  }
  return rc;
}

// closest point to origin for a 2-simplex
bool ON_3dSimplex::Closest2plex(ON_4dPoint& Bary) const
{
  bool rc = false;
  ON_3dVector N = FaceNormal(0);
  double N2 = N.LengthSquared();
  if (N2 > 0)
  {
    ON_3dPoint P3 = (m_V[0] * N)*N / N2;										// origin projected to Affine_Span < V_0, V_1, V_2 >
    int J = N.MaximumCoordinateIndex();

    ON_3dPoint Planar[3];		// We reduce to a planar closest point to origin problem
    for (int i = 0; i < 3; i++)
      Planar[i] = Vertex(i) - P3;

    // Finding the barycentric coordintes of the origin will guild the rest of the algorithm
    // We simplify this by projecting to Not J plane

    double DetM = 0.0;
    double C3[3];
    int j0 = (J + 1) % 3;
    int j1 = (J + 2) % 3;
    for (int i = 0; i < 3; i++)
    {
      int i0 = (i + 1) % 3;
      int i1 = (i + 2) % 3;
      C3[i] = Planar[i0][j0] * Planar[i1][j1] - Planar[i1][j0] * Planar[i0][j1];
      DetM += C3[i];
    }

    if (DetM != 0.0)
    {
      bool interior = true;
      for (int j = 0; interior && j < 3; j++)
        interior = SameSign(DetM, C3[j]);
      if (interior)
      {
        for (int i = 0; i < 3; i++)
          Bary[i] = C3[i] / DetM;
        rc = true;
      }
      else
      {
        for (int j = 0; j < 3; j++)
        {
          if (!SameSign(DetM, C3[j]))
          {
            ON_3dSimplex S(Planar[(j + 1) % 3], Planar[(j + 2) % 3]);		// S is a 1-simplex
            ON_4dPoint bary;
            if (S.GetClosestPointToOrigin(bary))
            {
              rc = true;
              bool OnEnd = (bary[0] == 1.0 || bary[1] == 1.0);
              Bary[j] = 0.0;
              Bary[3] = 0.0;
              for (int i = 0; i < 2; i++)
                Bary[(j + 1 + i) % 3] = bary[i];
              if (!OnEnd)
                break;
            }
          }
        }
      }
    }
  }
  return rc;
}

// closest point to a 3-simplex
bool ON_3dSimplex::Closest3plex(ON_4dPoint& Bary) const
{
  bool rc = false;
  //  Solving 
  //        [ V_0  V_1  V_2  V_3 ]          [ 0 ] 
  //  M*B = [  1    1    1    1  ]  * B  =  [ 1 ]

  int ind[3] = { 1,2,3 };
  double detM = 0.0;
  double C4[4];  // C4[j] = C_{4,j} is a cofactor of M
  double sign = 1.0;
  for (int j = 0; j < 4; j++)
  {
    C4[j] = sign * ON_TripleProduct(m_V[ind[0]], m_V[ind[1]], m_V[ind[2]]);
    if (j < 3)
    {
      ind[j] = j;  // {1,2,3}, {0,2,3}, {0,1,3}, {0,1,2}
      sign *= -1.0;
    }
    detM += C4[j];
  }


  if (detM != 0.0)
  {
    bool interior = true;
    int j = 0;
    for (j = 0; interior && j < 4; j++)
      interior = SameSign(detM, C4[j]);
    if (interior)
    {
      for (int i = 0; i < 4; i++)
        Bary[i] = C4[i] / detM;
      rc = true;
    }
    else
    {
      j--;
      double D2 = ON_DBL_MAX;		// best answer so far
      int N = 5;								// size of support
      do
      {
        if (!SameSign(detM, C4[j]))
        {
          ON_3dSimplex S = (*this);
          S.RemoveVertex(j);
          ON_4dPoint bary;
          if (S.Closest2plex(bary))
          {
            int n = 0;	// size of support
            for (int i = 0; i < 3; i++) if (bary[i] > 0)n++;
            if (n == 3)
            {
              for (int i = 3; i > j; i--)
                bary[i] = bary[i - 1];
              bary[j] = 0.0;
              Bary = bary;
              rc = true;
              break;
            }
            else
            {
              ON_3dVector cp = S.Evaluate(bary);
              double d2 = cp.LengthSquared();

              if (d2 < D2 || d2 == D2 && n < N)
              {
                D2 = d2;
                N = n;
                for (int i = 3; i > j; i--)
                  bary[i] = bary[i - 1];
                bary[j] = 0.0;
                rc = true;
                Bary = bary;
              }
            }
          }
          else
            rc = false;
        }
      } while (++j < 4);
    }
  }
  return rc;
}

bool ON_ConvexPoly::Standardize(ON_4dex& dex, ON_4dPoint& B)
{
  bool rc = true;
  ON_4dex rdex = { -1,-1,-1,-1 };	// results
  ON_4dPoint rB(0, 0, 0, 0);
  int ri = 0; // index into result
  for (int ii = 0; ii < 4; ii++)	// index in input
  {
    while ((dex[ii] < 0 || B[ii] == 0.0) && ii < 4) ii++;
    if (ii == 4)
      break;
    int j = 0;
    while (j < ri && rdex[j] != dex[ii]) j++;
    if (j == ri)
    {
      rdex[ri] = dex[ii];
      rB[ri++] = 0.0;
    }
    rB[j] += B[ii];
  }

  if (rc)
  {
    dex = rdex;
    B = rB;
  }
  return rc;
}


void ON_ConvexHullRef::Initialize(const ON_3dVector* V0, int n)
{
  m_n = n;
  m_v = *V0;
  m_is_rat = false;
  m_stride = 3;
}

void ON_ConvexHullRef::Initialize(const ON_4dPoint* V0, int  n)
{
  m_n = n;
  m_v = *V0;
  m_is_rat = true;
  m_stride = 4;
}

// style must be either not_rational or homogeneous_rational = 2,
void ON_ConvexHullRef::Initialize(const double* V0, ON::point_style style, int  count)
{
	if (style == ON::homogeneous_rational)
		Initialize(reinterpret_cast<const ON_4dPoint*>(V0), count);
	else
			Initialize(reinterpret_cast<const ON_3dVector*>(V0), count);
}

ON_ConvexHullRef::ON_ConvexHullRef(const ON_3dVector* V0, int n)
{
  m_n = n;
  m_v = *V0;
  m_is_rat = false;
  m_stride = 3;
}

ON_ConvexHullRef::ON_ConvexHullRef(const ON_3dPoint* P0, int n)
{
  m_n = n;
  m_v = *P0;
  m_is_rat = false;
  m_stride = 3;
}

ON_ConvexHullRef::ON_ConvexHullRef(const ON_4dPoint* V0, int  n)
{
  m_n = n;
  m_v = *V0;
  m_is_rat = true;
  m_stride = 4;
}

ON_ConvexHullRef::ON_ConvexHullRef(const double* V0, bool is_rat, int  n)
{
  m_n = n;
  m_v = V0;
  m_is_rat = is_rat;
  m_stride = is_rat ? 4 : 3;
}

ON_ConvexHullRef::ON_ConvexHullRef(const double* V0, bool is_rat, int  n, int stride)
{
  m_n = n;
  m_v = V0;
  m_is_rat = is_rat;
  m_stride = stride;
}

ON_3dVector ON_ConvexHullRef::Vertex(int j) const
{
  ON_3dVector v;
  if (m_is_rat)
  {
    ON_4dPoint hv = *(reinterpret_cast<const ON_4dPoint*>(m_v + m_stride*j));
    v = ON_3dVector(hv.EuclideanX(), hv.EuclideanY(), hv.EuclideanZ());
  }
  else
  {
    v = *(reinterpret_cast<const ON_3dVector*>(m_v + m_stride*j));
  }
  return v;
}

int ON_ConvexHullRef::SupportIndex(ON_3dVector W, int) const
{
  int j0 = 0;
  double dot = Vertex(0)*W;
  for (int j = 1; j < m_n; j++)
  {
    ON_3dVector v = Vertex(j);
    double d = v * W;
    if (d > dot)
    {
      dot = d;
      j0 = j;
    }
  }
  return j0;
}

double ON_ConvexHullRef::MaximumCoordinate() const
{
  return ON_MaximumCoordinate(m_v, 3, m_is_rat, m_n);
}

int ON_ConvexHullPoint2::AppendVertex(const ON_3dPoint& P) // return index of new vertex.  must set Adjacent Indicies.
{
  m_Vert.Append(P);
  Ref.Initialize(m_Vert, m_Vert.Count());
  return m_Vert.Count()-1;
}

void ON_ConvexHullPoint2::Empty()
{
	Ref.Initialize(m_Vert, 0);
	m_Vert.Empty();
}

double ON_ConvexHullPoint2::MaximumCoordinate() const
{
  return ON_MaximumCoordinate(m_Vert[0], 3, false, m_Vert.Count());
}

bool ClosestPoint(const ON_3dPoint P0, const ON_ConvexPoly& poly,
  ON_4dex& dex, ON_4dPoint& Bary, double atmost )
{
  ON_ConvexHullRef CvxPt(&P0, 1);
  // Set pdex to match the support of dex
  ON_4dex pdex = dex;
  for (int i = 0; i < 4; i++)
  {
    if (dex[i] >= 0) 
      pdex[i] = 0;
  }
  bool rc =  ClosestPoint(CvxPt, poly, pdex, dex, Bary, atmost);
  ON_ConvexPoly::Standardize(dex, Bary);
  return rc;
}

// MatchingSupport(A, B) retuns a positive number if
//  A[i]<0 iff B[i]<0 and at least one coordinate pair has valid indicies A[i]>=0 and B[i]>=0.
static int MatchingSupport(const ON_4dex& A, const ON_4dex& B)
{
  int nsup  = 0;
  int i =0;
  for (; i < 4; i++)
  {
    if ((A[i] < 0) != (B[i] < 0))
      break;
    if (A[i] >= 0)
      nsup++;
  }
  return (i == 4) ? nsup : -1;
}

static int Gregdebugcounter = 0;


static void DebugCounter()
{
#ifdef ON_DEBUG
  Gregdebugcounter++;
#endif
}

// Gilbert Johnson Keerthi  algorithm

// To supply an inital seed simplex Adex and Bdex must be valid and 
// have matching support specifically
//     Adex[i]<A.Count() and Bdex[i]<B.Count()   for all i
//     Adex[i]<0 iff Bdex[i]<0  for all i
//     Adex[i]>=0 for some i    for some i
// By satisfying this condition Adex and Bdex will define a simplex in A - B 
// Note the result of a ClosestPoint calculation Adex and Bdex satisfy these conditions 
bool ClosestPoint(const ON_ConvexPoly& A, const ON_ConvexPoly& B,
  ON_4dex& Adex, ON_4dex& Bdex, ON_4dPoint& Bary, double atmost)
{
  bool rc = false;
  if (A.Count() == 0 || B.Count() == 0)
    return false;

  int Aind[4] = { -1,-1,-1,-1 };
  int Bind[4] = { -1,-1,-1,-1 };
  ON_3dSimplex Simp;
  Bary = ON_4dPoint(1, 0, 0, 0);
  ON_3dVector v(0,0,0);


  // If Adex and Bdex are valid on entry we use them as an inital 
  // seed for the trial simplex.  This case is indicated by setting
  // bFirstPass 
  bool bFirstPass = false;       
  if (A.IsValid4Dex(Adex) && B.IsValid4Dex(Bdex) && MatchingSupport(Adex, Bdex)>0 )
  {
    // Set the initial condition for Aind, Bind and Simp from Adex and Bdex
    int j = 0;
    int i = 0;
    for (i = 0; i < 4; i++)
    {
      if (Adex[i] < 0 || Bdex[i] < 0)
        continue;
      int jj;
      for (jj = 0; jj < j; jj++)
      {
        if (Aind[jj] == Adex[i] && Bind[jj] == Bdex[i])
          break;
      }
      if (jj < j)
        break;
      Aind[j] = Adex[i];
      Bind[j] = Bdex[i];
      ON_3dVector vert = A.Vertex(Aind[j]) - B.Vertex(Bind[j]);
      Simp.AddVertex(vert);
      j++;
    }

    if( i==4) 
     bFirstPass = true;
    else
     bFirstPass = false;
  }
	
	bool done = false;
	double vlen = ON_DBL_MAX;
	double vlenlast = ON_DBL_MAX;
	while (!done)
	{
		if (!bFirstPass)
		{
      // Default initial simplex is a point  A.Vertex(0) - B.Vertex(0);
			Aind[0] = Bind[0] = 0;
			Aind[1] = Aind[2] = Aind[3] = -1;
			Bind[1] = Bind[2] = Bind[3] = -1;
			v = A.Vertex(0) - B.Vertex(0);
			vlenlast = ON_DBL_MAX;
			vlen = v.Length();

			Simp = ON_3dSimplex();
			Simp.AddVertex(v);
		}
		// The key to the implemetation of this algorithm is contained in Simplex::GetClosestPointToOrigin()

		//  These lines are for ON_3dSimplex
		const bool SimplexReindexing = true;

		// These lines are for ON_JSimplex
		//const bool SimplexReindexing = false;
		//ON_JSimplex Simp;

		double mu = 0.0;
		const double epsilon = 10000.0 * ON_EPSILON;

		int wA = 0, wB = 0;
		while (!done && (bFirstPass || vlen > 0))
		{
			ON_3dVector W;
			if (!bFirstPass)
			{
				wA = A.SupportIndex(-v, wA);
				wB = B.SupportIndex(v, wB);
				W = A.Vertex(wA) - B.Vertex(wB);
        ON_3dVector unit_v = 1 / vlen * v;
				double del = unit_v * W ;		// this is lower bound on the distance
				if (del > mu)
					mu = del;

				// 18-July-19  Considered Adding vlen>=vlenlast to ensure distance is decreasing
				//    See RH-47044 for a case that got hung up here
        // See RH-30343 for a case where the 2.0 factor is needed
        // WRONG!!!  RH-30343 again.  the 2.0 factor doest't work either
        // TODO: testing... If the support vertex is already in simplex were done
        int i = 0;  
        for (i = 0; i < 4; i++)
        {
          if (Aind[i] == wA && Bind[i] == wB) break;
        }

        // See 100818MatchPoints.3dm if Bary>0 then we are done since closest point is
        // in the interior of a Simplex 
        if (i < 4 || Simp.Count()==4)
          done = true;
        else
          done = ((vlen - mu) <= 2.0* mu *epsilon) || mu > atmost  || vlen >= vlenlast ;//TODO "1+" ?? got rid of it
      }
			if (!done)
			{
				// n0 is the index of the newly added vertex
				int n0;
				if (!bFirstPass)
				{
					n0 = Simp.Count();  // this is specific to ON_3dSimplex
					Simp.AddVertex(W);
					Aind[n0] = wA;
					Bind[n0] = wB;
				}
				else
					n0 = Simp.Count() - 1;



				if (Simp.GetClosestPointToOrigin(Bary))
				{
					DebugCounter();
					bFirstPass = false;
					v = Simp.Evaluate(Bary);
					vlenlast = vlen;
					vlen = v.Length();
					int imax = 3;
					if (SimplexReindexing)
						imax = n0;
					for (int i = imax; i >= 0; i--)
					{
						if (Bary[i] == 0.0)
						{
							Simp.RemoveVertex(i);
							if (SimplexReindexing)
							{
								/*  The JSimplex the indicies are fixed so the following step is not needed.
										the ON_3dSimplex type does change the index of verticies and this piece of
										code adjusts for this.*/
								for (int j = i; j < n0; j++)
								{
									Bary[j] = Bary[j + 1];
									Aind[j] = Aind[j + 1];
									Bind[j] = Bind[j + 1];
								}
								Bary[n0] = 0.0;
								n0--;
							}
						}
					}
				}
				else
				{
					// In this case I am going to terminte the iteration.  If this was a FirstPass with user supplied initial guess
					// then we restart the algorithm without the initial guess.
					break;
				}
			}
		}
		if (!done)
		{
			if (bFirstPass)
				bFirstPass = false;
			else
				done = true;

		}

    /* TODO
    RH-54751
    vlen is nearly 0. but it should be 0.0 
    If (0,0,0) s in the interior of A-B then solution is vlen == 0.0 
    */
    if (Simp.Count() == 4 && Simp.Volume() > ON_SQRT_EPSILON)
    {
      vlen = 0.0;
    }
		rc = (vlen <= atmost);
		if (rc)
		{
			if (SimplexReindexing)
			{
				if (Simp.Count() > 0)
				{
					for (int i = Simp.Count(); i < 4; i++)
					{
						Bary[i] = 0.0;
						Aind[i] = Bind[i] = -1;
					}
				}
			}


			Adex = ON_4dex(Aind[0], Aind[1], Aind[2], Aind[3]);
			Bdex = ON_4dex(Bind[0], Bind[1], Bind[2], Bind[3]);
		}
	}
  return rc;
}


// Is PQR a right hnd turn
static bool IsLeftTurn(const ON_2dPoint& P, const ON_2dPoint& Q, const ON_2dPoint& R)
{
	ON_2dVector A = R - Q;
	ON_2dVector B = P - Q;
	double det = A.x* B.y - B.x*A.y;
	return det > 0.0;
}


int ON_ConvexHull2d(const ON_SimpleArray<ON_2dPoint>& Pnt, ON_SimpleArray<ON_2dPoint>& Hull, ON_SimpleArray< int>* PntInd)
{
	int rval = -1;
	Hull.Empty();
	ON_SimpleArray<int> Ind(Pnt.Count());
	Ind.SetCount(Pnt.Count());
	if (PntInd)
		PntInd->Empty();
	if (!Pnt.Sort(ON::sort_algorithm::quick_sort, Ind,
		[](const ON_2dPoint* A, const ON_2dPoint* B) { return ON_2dPoint::Compare(*A, *B); }))
		return  rval;
	if (Pnt.Count() == 0)
		return rval;

	Hull.Append(Pnt[Ind[0]]);
	if (PntInd)
		PntInd->Append(Ind[0]);
	int fixed = 1;    // This is the count of Hull that is fixed
	int ri = 1;

	for (int inc = 1; inc >= -1; inc -= 2)
	{
		for ( /*empty*/; ri < Pnt.Count() && ri >= 0; ri += inc)
		{
			ON_2dPoint R = Pnt[Ind[ri]];

			if (Hull.Count() == fixed)
			{
				if (R != *Hull.Last())
				{
					Hull.Append(R);
					if (PntInd)
						PntInd->Append(Ind[ri]);
				}
			}
			else
			{
				int pi = Hull.Count() - 2;
				ON_2dPoint P = Hull[pi];
				ON_2dPoint Q = *Hull.Last();
				if (IsLeftTurn(P, Q, R))
				{
					Hull.Append(R);
					if (PntInd)
						PntInd->Append(Ind[ri]);
				}
				else
				{
					bool done = false;
					while (!done)
					{
						Hull.Remove();
						if (PntInd)
							PntInd->Remove();
						Q = P;
						done = (pi == ((inc==1)?0:fixed-1));
						if (!done)
						{
							P = Hull[--pi];
							done = IsLeftTurn(P, Q, R);
						}

					}
					Hull.Append(R);
					if (PntInd)
						PntInd->Append(Ind[ri]);
				}
			}
		}
		if (Hull.Count() == 1) {
			rval = 0;
			break;
		}
		fixed = Hull.Count();
		ri = Pnt.Count() - 2;
	}

	if (Hull.Count() == 2)
		rval = 1;
	else
		rval = 2;
	return rval;
}



