#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cmath>

using namespace std;
#include <acado_toolkit.hpp>
#include <acado_optimal_control.hpp>
#include <acado_code_generation.hpp>

USING_NAMESPACE_ACADO

int main( )
{
    USING_NAMESPACE_ACADO

	// prediction horizon and sampling time
	const float TEnd = 2;
    const double Ts = 1 / 15.0;

    // Variables:
	DifferentialState   x     ;  // position x
	DifferentialState   y     ;  // position y
	DifferentialState   z     ;  // position z
	DifferentialState   vx    ;  // velocity x 
    DifferentialState   vy    ;  // velocity y
    DifferentialState   z1    ;  // velocity z state z1
    DifferentialState   z2    ;  // velocity z state sz2
    DifferentialState   psi   ;  // angle roll
    DifferentialState   theta ;  // angle pitch
	DifferentialState   phi   ;  // angle yaw
	DifferentialState   y1    ;  // yaw state y1
	DifferentialState   y2    ;  // yaw state y2
	DifferentialState   dummy    ;  // yaw state y2
	
	// Control command
	Control             psid  ;  // command roll
    Control             thetad;  // command theta
    Control             vzd   ;  // command vertical velocity
	Control   			phird ;  // command yawrate
	// Slack variables
	Control 			sv_obs;  // slack variable for obstacle 

	OnlineData 			obs_x ;	 // obstacle's x
	OnlineData 			obs_y ;	 // obstacle's y

    const float     pi = 3.14159265359; // pi

    // Model equations:
	DiscretizedDifferentialEquation f(Ts); 

	f << next(  x  ) == x + 0.066259444564402 * vx + 0.009397246901320 * theta + 0.001484851818313 * thetad;
	f << next(  y  ) == y + 0.066222649190692 * vy + 0.009389533529518 * psi + 0.001486468522497 * psid;
    f << next(  z  ) == z + 1.067248658575422 * z1 - 0.100493327248945 * z2 + 0.001919730358230 * vzd;
	f << next(  vx ) == 0.987808262200150 * vx + 0.261743128622771 * theta + 0.12882065558 * thetad;
    f << next(  vy ) == 0.986709114307428 * vy - 0.261415513308866 * psi - 0.12893425756 * psid;
	f << next(  z1 ) == 0.814844314440081 * z1 - 0.127542464394678 * z2 + 0.003479876523758 * vzd;
    f << next(  z2 ) == 0.487262230640373 * z1 + 0.637135147383496 * z2 - 0.035522579058221 * vzd;
	f << next( psi ) == 0.637504888761587 * psi + 0.368558257859032 * psid;
    f << next(theta) == 0.638406532682654 * theta + 0.368171947039015 * thetad;
	f << next( phi ) == phi + 0.580946305515318 * y1 - 0.002380060951283 * y2;
	f << next(  y1 ) == 0.936465349398944 * y1 - 0.046804990378125 * y2 + 5.125900721967114e-04 * phird;
	f << next(  y2 ) == 0.236914366025587 * y1 + 0.820329015243120 * y2 - 0.013647220727940 * phird;
	f << next(  dummy ) == sv_obs;
	

    // Reference functions and weighting matrices:
	Function h, hN;
	h << psid << thetad << vzd << phird << sv_obs;
	hN << x << y << z << vx << vy << phi;

	DMatrix W = eye<double>(h.getDim());
	// W(4,4) = 1000;
	DMatrix WN = eye<double>(hN.getDim());
	WN(3,3) = 10;
	WN(4,4) = 10;
	WN(5,5) = 100;
	//
	// Optimal Control Problem
	//

	OCP ocp( 0.0,  TEnd , TEnd / Ts);

	Expression d2_obs = (x - obs_x) * (x - obs_x) + (y - obs_y) * (y - obs_y);

	ocp.setNOD(2);

	ocp.minimizeLSQ(W, h);
	ocp.minimizeLSQEndTerm(WN, hN);

	ocp.subjectTo( f );

	ocp.subjectTo( -pi / 36 <= psid <= pi / 36 ); 	// deg
	ocp.subjectTo( -pi / 36 <= thetad <= pi / 36 ); // deg
    ocp.subjectTo( -1 <= vzd <= 1 ); 				// deg
	ocp.subjectTo( -pi / 2 <= phird <= pi / 2 ); 	// 90 deg/s
	ocp.subjectTo( 2 <= d2_obs + sv_obs );
	ocp.subjectTo( 0.25 <= d2_obs);
	ocp.subjectTo( 0 <= sv_obs);

	// Export the code:
	OCPexport mpc( ocp );
	
	mpc.set( HESSIAN_APPROXIMATION, GAUSS_NEWTON);
    mpc.set( DISCRETIZATION_TYPE, MULTIPLE_SHOOTING);
    mpc.set( SPARSE_QP_SOLUTION, FULL_CONDENSING_N2); 
    mpc.set( INTEGRATOR_TYPE, INT_DT);
    mpc.set( QP_SOLVER, QP_QPOASES);
    mpc.set( HOTSTART_QP, YES);
    mpc.set( LEVENBERG_MARQUARDT, 1e-10);
    mpc.set( LINEAR_ALGEBRA_SOLVER, GAUSS_LU);
    // mpc.set( IMPLICIT_INTEGRATOR_NUM_ITS, 2);
	mpc.set( GENERATE_TEST_FILE, BT_TRUE);
    // mpc.set( CG_USE_OPENMP, NO);
    mpc.set( CG_HARDCODE_CONSTRAINT_VALUES, NO);
    mpc.set( CG_USE_VARIABLE_WEIGHTING_MATRIX, NO);

	if (mpc.exportCode( "mpc_flight_controller_export" ) != SUCCESSFUL_RETURN)
		exit( EXIT_FAILURE );

	mpc.printDimensionsQP( );

	return EXIT_SUCCESS;
}
