// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_GEOMETRY_AFFINEGEOMETRY_HH
#define DUNE_GEOMETRY_AFFINEGEOMETRY_HH

/** \file
 *  \brief  an implementation of the Geometry interface for affine geometries
 *  \author Martin Nolte
 */

#include <dune/common/fmatrix.hh>
#include <dune/common/fvector.hh>

#include <dune/geometry/type.hh>
#include <dune/geometry/genericgeometry/geometrytraits.hh>
#include <dune/geometry/genericgeometry/matrixhelper.hh>

namespace Dune
{

  // External Forward Declarations
  // -----------------------------

  template< class ctype, int dim >
  class ReferenceElement;

  template< class ctype, int dim >
  class ReferenceElements;



  // AffineGeometryTraits
  // --------------------

  template< class ct >
  struct AffineGeometryTraits
  {
    typedef GenericGeometry::MatrixHelper< GenericGeometry::DuneCoordTraits< ct > > MatrixHelper;

    struct UserData {};
  };



  // AffineGeometry
  // --------------

  template< class ct, int mydim, int cdim, class Traits = AffineGeometryTraits< ct > >
  class AffineGeometry
  {
    typedef AffineGeometry< ct, mydim, cdim, Traits > This;

  public:
    /** \brief type used for coordinates */
    typedef ct ctype;

    /** \brief dimension of the geometry (i.e., of the reference element) */
    static const int mydimension= mydim;
    /** \brief dimension of the world space */
    static const int coorddimension = cdim;

    /** \brief type of user data */
    typedef typename Traits::UserData UserData;

    /** \brief type for local coordinate vector */
    typedef FieldVector< ctype, mydimension > LocalCoordinate;
    /** \brief type for global coordinate vector */
    typedef FieldVector< ctype, coorddimension > GlobalCoordinate;

    /** \brief type for the transposed Jacobian matrix */
    typedef FieldMatrix< ctype, mydimension, coorddimension > JacobianTransposed;
    /** \brief type for the transposed inverse Jacobian matrix */
    typedef FieldMatrix< ctype, coorddimension, mydimension > JacobianInverseTransposed;

    // for compatibility, export the type JacobianInverseTransposed as Jacobian
    typedef JacobianInverseTransposed Jacobian;

    /** \brief type of reference element */
    typedef Dune::ReferenceElement< ctype, mydimension > ReferenceElement;

  protected:
    typedef Dune::ReferenceElements< ctype, mydimension > ReferenceElements;

  private:
    // helper class to compute a matrix pseudo inverse
    typedef typename Traits::MatrixHelper MatrixHelper;

    struct Storage
      : public UserData
    {
      Storage ( const ReferenceElement &refEl, const GlobalCoordinate &org,
                const JacobianTransposed &jt, const UserData &userData )
        : UserData( userData ),
          refElement( &refEl ),
          origin( org ),
          jacobianTransposed( jt )
      {
        integrationElement = MatrixHelper::template rightInvA< mydimension, coorddimension >( jacobianTransposed, jacobianInverseTransposed );
      }

      template< class CoordVector >
      Storage ( const ReferenceElement &refEl, const CoordVector &coordVector,
                const UserData &userData )
        : UserData( userData ),
          refElement( &refEl ),
          origin( coordVector[ 0 ] )
      {
        for( int i = 0; i < mydimension; ++i )
          jacobianTransposed[ i ] = coordVector[ i+1 ] - origin;
        integrationElement = MatrixHelper::template rightInvA< mydimension, coorddimension >( jacobianTransposed, jacobianInverseTransposed );
      }

      const ReferenceElement *refElement;
      GlobalCoordinate origin;
      JacobianTransposed jacobianTransposed;
      JacobianInverseTransposed jacobianInverseTransposed;
      ctype integrationElement;
    };

  public:
    /** \brief constructor
     *
     *  \param[in]  refElement          reference element for this geometry
     *  \param[in]  origin              image of the origin under the geometric mapping
     *  \param[in]  jacobianTransposed  constant transposed Jacobian matrix of the mapping
     *  \param[in]  userData            initial value for the user data
     *                                  (optional, if UserData is default constructable)
     */
    AffineGeometry ( const ReferenceElement &refElement, const GlobalCoordinate &origin,
                     const JacobianTransposed &jt, const UserData &userData = UserData() )
      : storage_( refElement, origin, jt, userData )
    {}

    /** \brief constructor
     *
     *  \param[in]  gt                  Dune::GeometryType describing the reference element
     *  \param[in]  origin              image of the origin under the geometric mapping
     *  \param[in]  jacobianTransposed  constant transposed Jacobian matrix of the mapping
     *  \param[in]  userData            initial value for the user data
     *                                  (optional, if UserData is default constructable)
     */
    AffineGeometry ( Dune::GeometryType gt, const GlobalCoordinate &origin,
                     const JacobianTransposed &jt, const UserData &userData = UserData() )
      : storage_( ReferenceElements::general( gt ), origin, jt, userData )
    {}

    /** \brief constructor
     *
     *  \param[in]  refElement          reference element for this geometry
     *  \param[in]  coordVector         vector of coordinates
     *  \param[in]  userData            initial value for the user data
     *                                  (optional, if UserData is default constructable)
     */
    template< class CoordVector >
    AffineGeometry ( const ReferenceElement &refElement, const CoordVector &coordVector,
                     const UserData &userData = UserData() )
      : storage_( refElement, coordVector, userData )
    {}

    /** \brief constructor
     *
     *  \param[in]  gt                  Dune::GeometryType describing the reference element
     *  \param[in]  coordVector         vector of coordinates
     *  \param[in]  userData            initial value for the user data
     *                                  (optional, if UserData is default constructable)
     */
    template< class CoordVector >
    AffineGeometry ( Dune::GeometryType gt, const CoordVector &coordVector,
                     const UserData &userData = UserData() )
      : storage_( ReferenceElements::general( gt ), coordVector, userData )
    {}

    /** \brief is this mapping affine? */
    bool affine () const { return true; }

    /** \brief obtain the name of the reference element */
    Dune::GeometryType type () const { return refElement().type(); }

    /** \brief obtain number of corners of the corresponding reference element */
    int corners () const { return refElement().size( mydimension ); }

    /** \brief obtain coordinates of the i-th corner */
    GlobalCoordinate corner ( int i ) const
    {
      return global( refElement().position( i, mydimension ) );
    }

    /** \brief obtain the centroid of the mapping's image */
    GlobalCoordinate center () const { return global( refElement().position( 0, 0 ) ); }

    /** \brief evaluate the mapping
     *
     *  \param[in]  local  local coordinate to map
     *
     *  \returns corresponding global coordinate
     */
    GlobalCoordinate global ( const LocalCoordinate &local ) const
    {
      GlobalCoordinate global( storage().origin );
      storage().jacobianTransposed.umtv( local, global );
      return global;
    }

    /** \brief evaluate the inverse mapping
     *
     *  \param[in]  global  global coorindate to map
     *
     *  \return corresponding local coordinate
     *
     *  \note The returned local coordinate y minimizes
     *  \code
     *  (global( x ) - y).two_norm()
     *  \endcode
     */
    LocalCoordinate local ( const GlobalCoordinate &global ) const
    {
      LocalCoordinate local;
      storage().jacobianInverseTransposed.mtv( global - storage().origin, local );
      return local;
    }

    /** \brief obtain the integration element
     *
     *  If the Jacobian of the mapping is denoted by $J(x)$, the integration
     *  integration element \f$\mu(x)\f$ is given by
     *  \f[ \mu(x) = \sqrt{|\det (J^T(x) J(x))|}.\f]
     *
     *  \param[in]  local  local coordinate to evaluate the integration element in
     *
     *  \returns the integration element \f$\mu(x)\f$.
     */
    ctype integrationElement ( const LocalCoordinate &local ) const
    {
      return storage().integrationElement;
    }

    /** \brief obtain the volume of the mapping's image */
    ctype volume () const
    {
      return storage().integrationElement * refElement().volume();
    }

    /** \brief obtain the transposed of the Jacobian
     *
     *  \param[in]  local  local coordinate to evaluate Jacobian in
     *
     *  \returns a reference to the transposed of the Jacobian
     *
     *  \note The returned reference is reused on the next call to
     *        JacobianTransposed, destroying the previous value.
     */
    const JacobianTransposed &jacobianTransposed ( const LocalCoordinate &local ) const
    {
      return storage().jacobianTransposed;
    }

    /** \brief obtain the transposed of the Jacobian's inverse
     *
     *  The Jacobian's inverse is defined as a pseudo-inverse. If we denote
     *  the Jacobian by \f$J(x)\f$, the following condition holds:
     *  \f[J^{-1}(x) J(x) = I.\f]
     */
    const JacobianInverseTransposed &jacobianInverseTransposed ( const LocalCoordinate &local ) const
    {
      return storage().jacobianInverseTransposed;
    }

    const UserData &userData () const { return storage_; }
    UserData &userData () { return storage_; }

  protected:
    const Storage &storage () const { return storage_; }
    Storage &storage () { return storage_; }

    const ReferenceElement &refElement () const { return *storage().refElement; }

  private:
    Storage storage_;
  };

} // namespace Dune

#endif // #ifndef DUNE_GEOMETRY_AFFINEGEOMETRY_HH
