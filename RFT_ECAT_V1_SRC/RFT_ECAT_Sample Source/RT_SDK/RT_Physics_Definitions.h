#ifndef __RT_PHYSICS_DEFINITIONS__
#define __RT_PHYSICS_DEFINITIONS__

#include <vector>
using namespace std;

// ---------------------------------------------------------------------- 
// POSITION				 ------------------------------------------------
// ---------------------------------------------------------------------- 
template <typename Type>
union POSITION_2Dunion{
	Type entry[2];
	struct {
		Type x;
		Type y;
	};
	unsigned char byte[sizeof(Type)* 2];
};
typedef POSITION_2Dunion<double>	POSITION_2D_double;
typedef POSITION_2Dunion<float>		POSITION_2D_float;
typedef POSITION_2Dunion<int>		POSITION_2D_int;
typedef POSITION_2Dunion<long>		POSITION_2D_long;

template <typename Type>
union POSITION_3Dunion{
	Type entry[3];
	struct {
		Type x;
		Type y;
		Type z;
	};
	unsigned char byte[sizeof(Type)* 3];
};
typedef POSITION_3Dunion<double>	POSITION_3D_double;
typedef POSITION_3Dunion<float>		POSITION_3D_float;
typedef POSITION_3Dunion<int>		POSITION_3D_int;
typedef POSITION_3Dunion<long>		POSITION_3D_long;

// ---------------------------------------------------------------------- 
// FT : force and torque ------------------------------------------------
// ---------------------------------------------------------------------- 
template <typename Type>
union FTunion{
	Type entry[6];
	struct {
		Type force[3];
		Type torque[3];
	};
	struct {
		Type Fx;
		Type Fy;
		Type Fz;
		Type Tx;
		Type Ty;
		Type Tz;
	};
	unsigned char byte[sizeof(Type)*6];
};
const string FT_name[6] = { "Fx", "Fy", "Fz", "Tx", "Ty", "Tz" };

typedef FTunion<double> FT_double;
typedef FTunion<float>	FT_float;

// ---------------------------------------------------------------------- 
// pose : position and orientation --------------------------------------
// ---------------------------------------------------------------------- 
template <typename Type>
union POSEunion{
	Type entry[6];
	struct {
		Type pos[3];
		Type rot[3];
	};
	struct {
		Type Px;
		Type Py;
		Type Pz;
		Type Rx;
		Type Ry;
		Type Rz;
	};
	unsigned char byte[sizeof(Type)* 6];
};
const string POSE_name[6] = { "Px", "Py", "Pz", "Rx", "Ry", "Rz" };

typedef POSEunion<double>	POSE_double;
typedef POSEunion<float>	POSE_float;


// ---------------------------------------------------------------------- 
// Velocity: Linear and Angular -----------------------------------------
// ---------------------------------------------------------------------- 
template <typename Type>
union VELunion{
	Type entry[6];
	struct {
		Type v[3];
		Type w[3];
	};
	struct {
		Type Vx;
		Type Vy;
		Type Vz;
		Type Wx;
		Type Wy;
		Type Wz;
	};
	unsigned char byte[sizeof(Type)* 6];
};
const string VEL_name[6] = { "vx", "vy", "yz", "wx", "wy", "wz" };

typedef VELunion<double>	VEL_double;
typedef VELunion<float>		VEL_float;


#endif // END OF FILE
