/*==========================================================================*/
/* Descirption 																*/
/*							                                                */
/*																			*/
/*							                                                */
/* Copyright:   Robotics Laboratory of ROBOTOUS, Co. Ltd.                   */
/*              www.robotous.com                                            */
/*==========================================================================*/

#ifndef _RT_COMMON_UTILITY_H_
#define _RT_COMMON_UTILITY_H_

#include "afxwin.h"
#include <atlimage.h> // for CImage
#include "Eigen/Dense"
using namespace Eigen;
#include <vector>
using namespace std;

/***************************************************************/
/*	Safe Release and Delete										/
/***************************************************************/
#ifndef SafeRelease
template <class T> 
void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}
#endif

#ifndef SafeDelete
template <class T>
void SafeDelete(T **ppT)
{
	if (*ppT)
	{
		delete (*ppT);
		*ppT = NULL;
	}
}
#endif
#ifndef SafeDeleteArray
template <class T>
void SafeDeleteArray(T **ppT)
{
	if (*ppT)
	{
		delete[] (*ppT);
		*ppT = NULL;
	}
}
#endif

/***************************************************************/
/*	Color Defines												/
/***************************************************************/
#define RT_WHITE		RGB(255,255,255)
#define RT_LIGHT_GREY	RGB(235,235,235)
#define RT_GREY			RGB(200,200,200)
#define RT_MEDIUM_GREY	RGB(128,128,128)
#define RT_DARK_GREY	RGB(64,64,64)
#define RT_BLACK		RGB(0,0,0)
#define RT_RED			RGB(255,0,0)
#define RT_MAGENTA		RGB(255,0,255)
#define RT_GREEN		RGB(0,255,0)
#define RT_BLUE			RGB(0,0,255)
#define RT_NICE_BLUE	RGB(0,128,192)
#define RT_DARK_BLUE	RGB(0,0,139)
#define RT_YELLOW		RGB(255,255,0)
#define RT_WINDOW_BG	((DWORD)0xFFFFFFFE)
#define COLOR_NOT_DEFINED ((DWORD)0xFFFFFFFF)

/***************************************************************/
/*	File Handling												/
/***************************************************************/

typedef struct{
	CString path;
	CString filename;		 // filename without path
	CString filename_no_ext; // filename without extension
	CString extension;
	CString tag;
}FILENAME_ATTRIBUTES;

bool get_attributes_from_filename(CString Filename, FILENAME_ATTRIBUTES& fna);

// make_folder_if_not_exist()
// 주어진 폴더명이 존재하는 지 확인하고 없으면 생성하는 함수
// 상위 폴더가 없으면 함께 생성
// return_msg:	생성결과 / 오류 메세지를 담을 CString 변수의 포인터
//				폴더가 존재하면 "" 스트링 반환
// 반환값:	폴더명이 존재하거나 생성이 정상적으로 이루어지면 true 
//			생성에 실패하거나 폴더명에 오류가 있으면 false
bool make_folder_if_not_exist(CString folder, CString *return_msg = NULL);

/***************************************************************/
/*	Image Handling												/
/***************************************************************/

// save the client area of a dialog window to the given format file
// pWnd : the pointer of window (dialog)
// filename: the name of file which will be saved the image (png)
// filetype :type of image file which will be saved. 
//		 (Gdiplus::ImageFormatPNG, Gdiplus::ImageFormatBMP, Gdiplus::ImageFormatJPEG ...)
bool save_window_client_as_image(CWnd *pWnd, CString filename, const GUID &filetype = Gdiplus::ImageFormatPNG);
// save the client area of a dialog window to the given format file
// pWnd : the pointer of window (dialog)
// rect_save: CRect of the saving area 
// filename: the name of file which will be saved the image (png)
// filetype :type of image file which will be saved. 
//		 (Gdiplus::ImageFormatPNG, Gdiplus::ImageFormatBMP, Gdiplus::ImageFormatJPEG ...)
bool save_window_client_as_image(CWnd *pWnd, CRect rect_save, CString filename, const GUID &filetype = Gdiplus::ImageFormatPNG);

/***************************************************************/
/*	Convert Angle Unit											/
/***************************************************************/

float  deg2rad(float  deg);
double deg2rad(double deg);
float  rad2deg(float  rad);
double rad2deg(double rad);

/***************************************************************/
/*	Assign Floating Point Dat to Eigen matrix / vector			/
/***************************************************************/

// Becuase the memory assinment of a 2D-array of normal c-array variable is different form Eigen's one,
// it is necessary to use the special macro/functions to exchange b.t.w c-array and Eigen Matrix

// Matrix mapped by c-style array-memory
#define MatrixXd_mapped_by_array(c_array, rows, cols)	(Map<MatrixXd>((double*) c_array, cols, rows)).transpose()
#define MatrixXf_mapped_by_array(c_array, rows, cols)	(Map<MatrixXf>((float*) c_array, cols, rows)).transpose()

// Matrix mapped to c-style array-memory
#define MatrixXd_mapped_to_array(c_array, rows, cols)	Map<Matrix<double,-1,-1,RowMajor>>((double*) c_array, rows, cols)
#define MatrixXf_mapped_to_array(c_array, rows, cols)	Map<Matrix<float,-1,-1,RowMajor>>((float*) c_array, rows, cols)

/* 
/	NOTE: In case of 1D matrix (e.g VectorXx), because there is no difference in memory assignment, just use Map class    
/	----------------------
/	Examples : How to use
/	----------------------
/	double g[2][3] = { { 21, 22, 23 }, { 24, 25, 26 } };
/	double h[2][3];
/	MatrixXd mat;
/
/	mat = MatrixXd_mapped_by_array(g, 2, 3);
/	MatrixXd_mapped_to_array(h, 2, 3) = mat;
*/

// An another choice to solve this confusion, initially define Matrix as 

typedef Matrix<double, -1, -1, RowMajor>	MatrixXd_RM; // Define MatrixXd as row major
typedef Matrix<float, -1, -1, RowMajor>		MatrixXf_RM; // Define MatrixXf as row major
typedef Matrix<int, -1, -1, RowMajor>		MatrixXi_RM; // Define MatrixXi as row major


/***************************************************************/
/*	Math.Statistics												/ 
/***************************************************************/

/*  get_matrix_mean() 
*
*	For an (n x m)matrix, 
*	calculate the mean(average)s of the column vectors.
*	return (m x 1) column vector 
*/
VectorXd get_matrix_mean(MatrixXd& mat);	 
VectorXf get_matrix_mean(MatrixXf& mat);

/*  get_mean()
*
*	For a floating-point array,
*	return mean(average) value
*/
float  get_mean(float *dp, int size);
double get_mean(double *dp, int size);

#define get_matrix_average get_matrix_mean		 
#define get_average get_mean		 

/*  get_matrix_stdev()
*   For an (n x m)matrix,
*	calculate the standard deviation(s) of the column vectors.
*	return (m x 1) column vector
*/
VectorXd get_matrix_stdev(MatrixXd& mat);
VectorXf get_matrix_stdev(MatrixXf& mat);


template <typename T>
void data2char_buffer(T& data, char* p_buf)
{
	memcpy(p_buf, &data, sizeof(T));
}

template <typename T>
T get_saturated_val(T value, T limit)
{
	if (limit < 0)
		limit = -limit;

	if (value > limit)
		value = limit;
	else if (value < (-limit))
		value =  -limit;
	return value;
}
template <typename T>
void saturate(T& value, T limit)
{
	if (limit < 0)
		limit = -limit;

	if (value > limit)
		value = limit;
	else if (value < (-limit))
		value = -limit;
}
// value 를 v1, v2사이값으로 한정시키다.
template <typename T>
void saturate(T& value, T v1, T v2)
{
	T max, min;
	if (v1>v2)
	{
		max = v1;	min = v2;
	}
	else
	{
		max = v2;	min = v1;
	}
	if (value > max)
		value = max;
	else if (value < min)
		value = min;
}


template <typename T>
void updateMinMax(T& min, T& max, T value)
{
	if (value > max)
		max = value;
	else if (value < min)
		min = value;
}
template <typename T>
void updateMin(T& min, T value)
{
	if (value < min)
		min = value;
}
template <typename T>
void updateMax(T& max, T value)
{
	if (value > max)
		max = value;
}

template <typename T>
void get_min_max(T& min, T& max, T* raw_data, int num_of_raw_data)
{
	min = max = raw_data[0];
	for (int i = 1; i < num_of_raw_data; i++)
		updateMinMax(min, max, raw_data[i]);
}


template <typename T>
bool is_value_in_range(T& value, T min, T max)
{
	if ((value >= min) && (value <= max))
		return  true;
	else
		return false;
}
template <typename T>
bool is_value_in_range(T* value, T* min, T* max)
{
	if ((value >= min) && (value <= max))
		return  true;
	else
		return false;
}

template <typename T>
void exchange(T& value1, T& value2)
{
	T temp;
	temp = value1;
	value1 = value2;
	value2 = temp;
}

#endif//_RT_COMMON_UTILITY_H_
// End of File
