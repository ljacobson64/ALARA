/* $Id: Chain.h,v 1.14 2007-12-12 06:06:14 phruksar Exp $ */
#include "alara.h"

#ifndef CHAIN_H 
#define CHAIN_H

#define INITMAXCHAINLENGTH 25

/** \brief This class stores the information about a particular chain as
 *         the chains are being created and solved.
 *   
 *  Normally, a single chain object will exist for each root isotope,
 *  exisiting only while that root is being solved.
 */

class Chain
{
protected:
  /// The tolerance limited to choose when to truncate chains based
  /// on main isotopes
  static double truncLimit;

  /// The tolerance limitied to choose when to ignore sub-trees
  /// based on main isotopes
  static double ignoreLimit;

  /// The threshold relative concentration within a mixture that 
  /// defines whether or not a root isotope will be treated as an 
  /// impurity or main isotope.
  static double impurityDefn;

  /// The tolerance limited to choose when to truncate chains based
  /// on impurity isotopes 
  static double impurityTruncLimit;

  /// The tolerance limited to choose when to ignore sub-trees based
  /// on impurity isotopes
  static double impurityIgnoreLimit;

  /// Flag indicating whether this is a forward or reverse calculation
  static int mode;

  /// A counter which simply tracks the length of the current chain.
  /** By convention, it will always be 1 greater than the rank of the
      bottom isotope in the chain. */
  int chainLength;
        
  /// This indicates the rank of the first isotope which is different
  /// from the last chain.
  /** This is important when manipulating the transfer and decay matrices;
      since they are lower triangular matrices, only the new elements,
      i.e. those corresponding to new isotopes, need to be changed. */
  int newRank;
        
  /// This indicates the rank of the first isotope to be tallied in this
  /// chain.
  /** A given isotope is tallied only immediately before it is
      removed from the chain.  Therefore, this isotope and all
      successive isotopes in the chain must currently be processing
      their last daughter isotope.  This is the condition which
      establishes which isotope is indicated by 'setRank'. */
  int setRank;

  /// Boolean indicates whether or not we are solving a reference/
  /// truncation calculation or a full solution.
  int solvingRef;

  /// If an isotope is involved in a loop, the last iteration of
  /// the loop need not be treated with loop solutions.
  /** This array tracks the beginning of the last loop iteration
      for each isotope.  In many (most?) situations this will be
      set to the default value, -1, indicating that this isotope
      is not involved in any loops. */
  int *loopRank;

  /// A list of pointers to the various rate vectors related to
  /// the chain.
  /** For each isotope, there is a pointer to the production rate
      vector and the total destruction rate vector for both
      transmutation and pure decay.  These are later collapsed
      with the fluxes as required. */
  double **rates;

  /// An array of scalar rates, calculated by collapsing the rate
  /// vectors pointed to by 'rates' with the fluxes of a given 
  /// interval.
  /** For each isotope in a chain and each flux in a problem, there
      is a scalar rate for production and destruction in both
      transmutation and pure decay.  A single 1-D array is used to
      store 3-D information to minimise memory allocation activity.
      The organization is as follows:
      Block P[chainLength * numFluxes] : transmutation production rates
      Block d[chainLength * numFluxes] : transmutation destruction rates
      Block L[chainLength] : decay production rates
      Block l[chainLength] : decay destruction rates
      For Block's P and d, the rates are sorted in numFluxes segments,
      each of lengh chainLength. */
  double *colRates;

  /// One of four points that simply point into the colRates vector at 
  /// the beginning of the blocks defined above to give 4 apparent rate
  /// arrays. This is a simple convenience measure.
  double *P;

  /// One of four points that simply point into the colRates vector at 
  /// the beginning of the blocks defined above to give 4 apparent rate
  /// arrays. This is a simple convenience measure.
  double *d;
        
  /// One of four points that simply point into the colRates vector at 
  /// the beginning of the blocks defined above to give 4 apparent rate
  /// arrays. This is a simple convenience measure.
  double *L;

  /// One of four points that simply point into the colRates vector at 
  /// the beginning of the blocks defined above to give 4 apparent rate
  /// arrays. This is a simple convenience measure.
  double *l;
 
  /// One of two pointers that simply point to truncLimit (ignoreLimit)
  /// or impurityTruncLimit (impurityIgnoreLimit) as appropriate for 
  /// the current chain being solved.
  double chainTruncLimit;
                
  /// One of two pointers that simply point to truncLimit (ignoreLimit)
  /// or impurityTruncLimit (impurityIgnoreLimit) as appropriate for 
  /// the current chain being solved.
  double chainIgnoreLimit;

  /// This points to the root isotope of this chain.
  /** It is used primarily for knowing where to tally the results of 
      the calculation. */
  Root *root;

  /// This points to the last isotope in the chain. 
  /** This last node is the object through which the chain growth and 
      truncation functions are accessed. */
  Node *node;
 
  /// This is a reference interval which stores the maximum group-wise
  /// flux for truncation calculations.
  Volume *reference;

  /// This simply indcates the current length of the various arrays
  /// and vectors.
  /** To avoid reallocating space with each change in the
      chain's length, these vectors are doubled and halved when
      appropriate.  When the vector is too small, it is doubled, and
      when the vector is larger than the default initial value
      INITMAXCHAINLENGTH and less than 1/4 full, it is halved. (see
      resizeRates()) */
  int maxChainLength;
  
  /// This function establishes the truncation state following the
  /// extension of a chain.
  void setState(topSchedule*);
  
  /// This function performs the doubling of the rate vectors 
  /// discussed in the description of 'maxChainLength'.
  void expandRates();

  /// This function performs the halving of the rate vectors 
  /// discussed in the description of 'maxChainLength'.
  void compressRates();

  /// This function determines whether to double, halve or leave 
  /// the rates vectors following the extension or truncation of
  /// the chain.
  void resizeRates();

public:
  /// This function reads the truncation tolerance information from 
  /// the input file attached to the stream reference passed in the 
  /// first argument.
  static void getTruncInfo(istream&);

  // NEED COMMENT
  static void getIgnoreInfo(istream&);

  /// This function reads the impurtiy threshold and the truncation
  /// tolerance information for impurities from the input file 
  /// attached to the stream reference passed in the first argument.
  static void getImpTruncInfo(istream&);

  /// This function sets 'mode' to the constant defining the 
  /// reverse calculation mode and calls NuclearData::modeReverse().
  static void modeReverse();

  /// Default constructor 
  Chain(Root *newRoot=NULL,topSchedule* top=NULL);

  /// The copy constructor 
  Chain(const Chain&);

  /// Destructor deletes all the vector storage associated with this
  /// class and the reference volume.
  ~Chain();

  /// Overloaded assignment operator  
  Chain& operator=(const Chain&);

  /// This recursive function hold the primary responsibility for
  /// managing the growth and pruning of the tree/chains.
  int build(topSchedule*);

  /// This function initializes the 'colRates' vector and sets the
  /// non-flux dependent values.
  void setupColRates();

  /// This function folds the vector rates pointed to by the 'rates'
  /// vector with the fluxes contained in the VolFlux list and fills
  /// 'colRates' with the scalar results.
  void collapseRates(VolFlux*);

  /// This function is the workhorse of the solution phase.
  void fillTMat(Matrix&, double, int);


  /// This function multiplies the two lower triangular matrices
  /// specified in the second and third arguments and assigns the 
  /// result to the first argument. 
  void mult(Matrix&, Matrix&, Matrix&);

  /// This function fills the necessary elements of a decay matrix,
  /// referenced by the first argument, using the decay time of the
  /// second argument.
  void setDecay(Matrix& D, double time);

  /// Calls interface routine of object pointed to by 'root' to get,
  /// and return, its KZA value
  int getRoot();

  /// Function steps through chain to rank passed in argument and calls
  /// the interface function of that Node object to get, and return, its
  /// KZA value.
  int getKza(int);

  /// Inline function provides access to the 'setRank' variable.
  int getSetRank() { return setRank; };

  /// Inline function provides access to the 'chainLength' variable.
  int getChainLength() { return chainLength; };
};

#endif
